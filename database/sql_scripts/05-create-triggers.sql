\connect photo_center_db

-- Функция для пересчета overall_price в таблице orders
CREATE OR REPLACE FUNCTION trg_recalculate_order_overall_price()
RETURNS TRIGGER AS $$
DECLARE
    v_order_id INT;
    v_new_overall_price NUMERIC(10, 2);
    v_client_discount NUMERIC(5, 2);
    v_is_urgent BOOLEAN;
    v_film_development_service_type_id INT;
    v_outlet_id INT;
    v_film_price_for_outlet NUMERIC(10, 2);
    v_is_film_bought_in_outlet BOOLEAN;
    v_film_service_order_id INT;
    v_film_code VARCHAR(255);
BEGIN
    -- Определяем order_id в зависимости от таблицы, вызвавшей триггер
    IF TG_TABLE_NAME = 'service_orders' THEN
        v_order_id := COALESCE(NEW.order_id, OLD.order_id);
    ELSIF TG_TABLE_NAME = 'print_orders' THEN
        v_order_id := COALESCE(NEW.order_id, OLD.order_id);
    ELSIF TG_TABLE_NAME = 'frames' THEN
        SELECT po.order_id INTO v_order_id FROM print_orders po WHERE po.id = COALESCE(NEW.print_order_id, OLD.print_order_id);
    ELSIF TG_TABLE_NAME = 'clients' THEN
        -- Для клиентов, нам нужно найти все их заказы и пересчитать overall_price
        -- Это может быть дорого для большого количества заказов.
        -- Для демонстрации, мы пересчитываем только для одного заказа,
        -- но в реальной системе может потребоваться асинхронная обработка или более сложная логика.
        IF TG_OP = 'UPDATE' AND OLD.discount IS DISTINCT FROM NEW.discount THEN
            FOR v_order_id IN (SELECT id FROM orders WHERE client_id = NEW.id) LOOP
                PERFORM trg_recalculate_order_overall_price_for_order(v_order_id);
            END LOOP;
        END IF;
        RETURN NEW;
    ELSIF TG_TABLE_NAME = 'print_discounts' THEN
        -- Для скидок на печать, нужно найти все заказы, использующие эту скидку
        IF TG_OP = 'UPDATE' AND OLD.discount IS DISTINCT FROM NEW.discount THEN
            FOR v_order_id IN (SELECT po.order_id FROM print_orders po WHERE po.print_discount_id = NEW.id) LOOP
                PERFORM trg_recalculate_order_overall_price_for_order(v_order_id);
            END LOOP;
        END IF;
        RETURN NEW;
    END IF;

    -- Если order_id не был определен (например, для clients или print_discounts,
    -- которые обрабатываются отдельно или через вспомогательную функцию)
    IF v_order_id IS NULL THEN
        RETURN NEW;
    END IF;

    -- Вызов вспомогательной функции для пересчета конкретного заказа
    PERFORM trg_recalculate_order_overall_price_for_order(v_order_id);

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION trg_recalculate_order_overall_price_for_order(p_order_id INT)
RETURNS VOID AS $$
DECLARE
    v_new_overall_price NUMERIC(10, 2);
    v_client_discount NUMERIC(5, 2);
    v_is_urgent BOOLEAN;
    v_outlet_id INT;
    v_film_development_service_type_id INT;
    v_film_price_for_outlet NUMERIC(10, 2);
    v_is_film_bought_in_outlet BOOLEAN;
    v_film_service_order_id INT;
    v_film_item_id INT;
    v_order_record RECORD; -- Добавляем переменную для записи заказа
BEGIN
    -- Получаем текущие данные заказа и БЛОКИРУЕМ строку orders для обновления
    SELECT o.is_urgent, o.outlet_id, c.discount
    INTO v_is_urgent, v_outlet_id, v_client_discount
    FROM orders o
    JOIN clients c ON o.client_id = c.id
    WHERE o.id = p_order_id
    FOR UPDATE OF o; -- Блокируем строку в таблице orders

    -- Инициализируем новую общую стоимость
    v_new_overall_price := 0;

    SELECT COALESCE(SUM(so.count * st.price * CASE WHEN v_is_urgent THEN 2.0 ELSE 1.0 END), 0)
    INTO v_new_overall_price
    FROM service_orders so
    JOIN service_types st ON so.service_type_id = st.id
    WHERE so.order_id = p_order_id;

    -- Логика учета бесплатной проявки пленки
    -- Аналогично, если films или delivery_items могут быть изменены, подумайте о FOR UPDATE.
    SELECT id INTO v_film_development_service_type_id
    FROM service_types
    WHERE name = 'Проявка пленки';

    IF v_film_development_service_type_id IS NOT NULL THEN
        FOR v_film_service_order_id, v_film_item_id IN (
            SELECT so_fd.id, f.item_id
            FROM service_orders so_fd
            JOIN films f ON so_fd.id = f.service_order_id
            WHERE so_fd.order_id = p_order_id
              AND so_fd.service_type_id = v_film_development_service_type_id
        ) LOOP
            IF v_film_item_id IS NOT NULL THEN
                SELECT EXISTS (
                    SELECT 1
                    FROM delivery_items di
                    JOIN deliveries d ON di.delivery_id = d.id
                    JOIN storages s ON d.storage_id = s.id
                    WHERE s.outlet_id = v_outlet_id
                      AND di.item_id = v_film_item_id
                ) INTO v_is_film_bought_in_outlet;

                IF v_is_film_bought_in_outlet THEN
                    SELECT price INTO v_film_price_for_outlet
                    FROM service_types
                    WHERE id = v_film_development_service_type_id;

                    v_new_overall_price := v_new_overall_price - COALESCE(v_film_price_for_outlet, 0);
                END IF;
            END IF;
        END LOOP;
    END IF;

    -- Добавляем стоимость печати с учетом скидок
    SELECT v_new_overall_price + COALESCE(SUM(
        f.amount * pp.price * (1 - COALESCE(pd.discount, 0) / 100.0) * (1 - COALESCE(v_client_discount, 0) / 100.0)
    ), 0)
    INTO v_new_overall_price
    FROM print_orders po
    JOIN frames f ON po.id = f.print_order_id
    JOIN print_prices pp ON f.print_price_id = pp.id
    LEFT JOIN print_discounts pd ON po.print_discount_id = pd.id
    WHERE po.order_id = p_order_id;

    -- Обновляем overall_price в таблице orders
    -- Эта UPDATE будет использовать уже полученную блокировку
    UPDATE orders
    SET overall_price = v_new_overall_price
    WHERE id = p_order_id;
END;
$$ LANGUAGE plpgsql;

-- Триггеры AFTER INSERT, UPDATE, DELETE на service_orders
CREATE TRIGGER trg_after_service_orders_change
AFTER INSERT OR UPDATE OR DELETE ON service_orders
FOR EACH ROW
EXECUTE FUNCTION trg_recalculate_order_overall_price();

-- Триггеры AFTER INSERT, UPDATE, DELETE на print_orders
CREATE TRIGGER trg_after_print_orders_change
AFTER INSERT OR UPDATE OR DELETE ON print_orders
FOR EACH ROW
EXECUTE FUNCTION trg_recalculate_order_overall_price();

-- Триггеры AFTER INSERT, UPDATE, DELETE на frames
CREATE TRIGGER trg_after_frames_change
AFTER INSERT OR UPDATE OR DELETE ON frames
FOR EACH ROW
EXECUTE FUNCTION trg_recalculate_order_overall_price();

-- Триггер AFTER UPDATE на clients для изменения скидок клиентов
CREATE TRIGGER trg_after_clients_discount_change
AFTER UPDATE OF discount ON clients
FOR EACH ROW
WHEN (OLD.discount IS DISTINCT FROM NEW.discount)
EXECUTE FUNCTION trg_recalculate_order_overall_price();

-- Триггер AFTER UPDATE на print_discounts для изменения скидок на печать
CREATE TRIGGER trg_after_print_discounts_change
AFTER UPDATE OF discount ON print_discounts
FOR EACH ROW
WHEN (OLD.discount IS DISTINCT FROM NEW.discount)
EXECUTE FUNCTION trg_recalculate_order_overall_price();

-- Триггер для поддержания актуального количества товаров на складе
-- Функция для обновления количества на складе
CREATE OR REPLACE FUNCTION trg_update_storage_quantity()
RETURNS TRIGGER AS $$
DECLARE
    v_storage_id INT;
    v_item_id INT;
    v_quantity_change INT;
    v_order_outlet_id INT;
BEGIN
    IF TG_TABLE_NAME = 'delivery_items' THEN
        -- Поступление товаров
        SELECT d.storage_id INTO v_storage_id FROM deliveries d WHERE d.id = NEW.delivery_id;
        v_item_id := NEW.item_id;
        v_quantity_change := NEW.quantity;

        INSERT INTO storage_items (quantity, item_id, storage_id)
        VALUES (v_quantity_change, v_item_id, v_storage_id)
        ON CONFLICT (item_id, storage_id) DO UPDATE 
        SET quantity = storage_items.quantity + EXCLUDED.quantity;

    ELSIF TG_TABLE_NAME = 'service_orders' THEN
        -- Использование товаров для услуг
        -- Определяем storage_id, связанный с outlet_id заказа
        SELECT o.outlet_id INTO v_order_outlet_id FROM orders o WHERE o.id = NEW.order_id;
        SELECT s.id INTO v_storage_id FROM storages s WHERE s.outlet_id = v_order_outlet_id;

        IF v_storage_id IS NULL THEN
            RAISE EXCEPTION 'Не найдено хранилище для торговой точки заказа ID: %', v_order_outlet_id;
        END IF;

        -- Определяем товары, необходимые для данной service_type
        FOR v_item_id, v_quantity_change IN
            SELECT stni.item_id, stni.count * NEW.count
            FROM service_types_needed_items stni
            WHERE stni.service_type_id = NEW.service_type_id
        LOOP
            UPDATE storage_items
            SET quantity = quantity - v_quantity_change
            WHERE item_id = v_item_id AND storage_id = v_storage_id;

            -- Удаляем запись, если количество стало 0 или меньше (опционально)
            DELETE FROM storage_items
            WHERE item_id = v_item_id AND storage_id = v_storage_id AND quantity <= 0;
        END LOOP;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Создаем UNIQUE ограничение для (item_id, storage_id) в storage_items, если его нет
-- Это необходимо для корректной работы ON CONFLICT в триггере
DO $$ BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_storage_items_item_storage') THEN
        ALTER TABLE storage_items ADD CONSTRAINT uq_storage_items_item_storage UNIQUE (item_id, storage_id);
    END IF;
END $$;


CREATE TRIGGER trg_after_delivery_items_change
AFTER INSERT OR UPDATE ON delivery_items
FOR EACH ROW
EXECUTE FUNCTION trg_update_storage_quantity();

CREATE TRIGGER trg_after_service_orders_items_use
AFTER INSERT OR UPDATE ON service_orders
FOR EACH ROW
EXECUTE FUNCTION trg_update_storage_quantity();


-- Триггер для предотвращения превышения емкости склада
CREATE OR REPLACE FUNCTION trg_check_storage_capacity()
RETURNS TRIGGER AS $$
DECLARE
    v_current_total_quantity INT;
    v_new_total_quantity INT;
    v_storage_capacity INT;
    v_item_id INT;
    v_storage_id INT;
    v_quantity_change INT;
BEGIN
    IF TG_OP = 'INSERT' THEN
        v_item_id := NEW.item_id;
        v_storage_id := NEW.storage_id;
        v_quantity_change := NEW.quantity;
    ELSIF TG_OP = 'UPDATE' THEN
        v_item_id := NEW.item_id;
        v_storage_id := NEW.storage_id;
        v_quantity_change := NEW.quantity - OLD.quantity;
    ELSE
        -- Для DELETE нет необходимости проверять емкость
        RETURN OLD;
    END IF;

    SELECT capacity INTO v_storage_capacity FROM storages WHERE id = v_storage_id;

    -- Суммарное количество товаров на данном складе, исключая текущий item_id для UPDATE
    SELECT COALESCE(SUM(quantity), 0)
    INTO v_current_total_quantity
    FROM storage_items
    WHERE storage_id = v_storage_id
      AND (TG_OP <> 'UPDATE' OR item_id <> OLD.item_id); -- Исключаем OLD.quantity для данного item_id при UPDATE

    v_new_total_quantity := v_current_total_quantity + v_quantity_change;

    IF v_new_total_quantity > v_storage_capacity THEN
        RAISE EXCEPTION 'Превышена емкость склада (ID: %). Допустимая емкость: %, текущая + новое количество: %', v_storage_id, v_storage_capacity, v_new_total_quantity;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_before_storage_items_insert_update
BEFORE INSERT OR UPDATE ON storage_items
FOR EACH ROW
EXECUTE FUNCTION trg_check_storage_capacity();


-- Триггер для установки скидки профессиональным клиентам
CREATE OR REPLACE FUNCTION trg_set_professional_discount()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.is_professional IS TRUE AND (OLD.is_professional IS DISTINCT FROM NEW.is_professional OR COALESCE(NEW.discount, 0) = 0) THEN
        NEW.discount := 10.00; -- Устанавливаем скидку по умолчанию для профессионалов
    ELSIF NEW.is_professional IS FALSE AND OLD.is_professional IS TRUE THEN
        NEW.discount := 0.00; -- Сбрасываем скидку, если клиент перестает быть профессионалом
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_before_clients_update_professional
BEFORE INSERT OR UPDATE OF is_professional, discount ON clients
FOR EACH ROW
EXECUTE FUNCTION trg_set_professional_discount();


-- Триггер для обеспечения приема срочных заказов только в филиалах
CREATE OR REPLACE FUNCTION trg_enforce_urgent_orders_at_branches()
RETURNS TRIGGER AS $$
DECLARE
    v_outlet_type_name VARCHAR(255);
BEGIN
    IF NEW.is_urgent IS TRUE THEN
        SELECT ot.name
        INTO v_outlet_type_name
        FROM outlets o
        JOIN outlet_types ot ON o.type_id = ot.id
        WHERE o.id = NEW.outlet_id;

        IF v_outlet_type_name IS NULL OR v_outlet_type_name <> 'branch' THEN
            RAISE EXCEPTION 'Срочные заказы могут быть приняты только в филиалах. Текущий тип торговой точки: %', COALESCE(v_outlet_type_name, 'Неизвестен');
        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_before_orders_insert_update_urgent
BEFORE INSERT OR UPDATE OF is_urgent, outlet_id ON orders
FOR EACH ROW
EXECUTE FUNCTION trg_enforce_urgent_orders_at_branches();

---
-- Простые хранимые процедуры (функции)
---

-- 1. Получить цену услуги по ее ID
CREATE OR REPLACE FUNCTION sp_get_service_price(
    p_service_type_id INT
)
RETURNS NUMERIC(10, 2) AS $$
DECLARE
    v_price NUMERIC(10, 2);
BEGIN
    SELECT price INTO v_price FROM service_types WHERE id = p_service_type_id;
    RETURN v_price;
END;
$$ LANGUAGE plpgsql;

-- 2. Получить ID типа торговой точки по ее названию
CREATE OR REPLACE FUNCTION sp_get_outlet_type_id(
    p_outlet_type_name VARCHAR(255)
)
RETURNS INT AS $$
DECLARE
    v_id INT;
BEGIN
    SELECT id INTO v_id FROM outlet_types WHERE name = p_outlet_type_name;
    RETURN v_id;
END;
$$ LANGUAGE plpgsql;

-- 3. Проверить, была ли пленка с данным кодом куплена в указанной торговой точке
-- (Предполагается, что 'items.name' может хранить 'films.code' для отслеживания покупок)
CREATE OR REPLACE FUNCTION sp_check_film_bought_in_outlet(
    p_film_code VARCHAR(255),
    p_outlet_id INT
)
RETURNS BOOLEAN AS $$
DECLARE
    is_bought BOOLEAN;
BEGIN
    SELECT EXISTS (
        SELECT 1
        FROM delivery_items di
        JOIN deliveries d ON di.delivery_id = d.id
        JOIN storages s ON d.storage_id = s.id
        JOIN items i ON di.item_id = i.id
        WHERE s.outlet_id = p_outlet_id
        AND i.name = p_film_code -- Предполагаем, что name в items может быть кодом пленки
    ) INTO is_bought;
    RETURN is_bought;
END;
$$ LANGUAGE plpgsql;

-- 4. Получить текущую скидку клиента по его ID
CREATE OR REPLACE FUNCTION sp_get_client_discount(
    p_client_id INT
)
RETURNS NUMERIC(5, 2) AS $$
DECLARE
    v_discount NUMERIC(5, 2);
BEGIN
    SELECT discount INTO v_discount FROM clients WHERE id = p_client_id;
    RETURN v_discount;
END;
$$ LANGUAGE plpgsql;