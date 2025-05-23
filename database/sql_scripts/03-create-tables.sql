-- Филиалы
CREATE TABLE branches (
    outlet_id INT NOT NULL UNIQUE PRIMARY KEY,
    CONSTRAINT fk_branch_outlet FOREIGN KEY (outlet_id)
        REFERENCES outlets(id) ON DELETE CASCADE
);

-- Типы торговых точек
CREATE TABLE outlet_types (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE
);

-- Здания торговых точек
CREATE TABLE outlets (
    id SERIAL PRIMARY KEY,
    address VARCHAR(255),
    num_workers INT NOT NULL CHECK (num_workers > 0),
    type_id INT NOT NULL UNIQUE,
    CONSTRAINT fk_outlet_type FOREIGN KEY (type_id)
        REFERENCES outlet_types(id) ON DELETE RESTRICT
);

-- Киоски
CREATE TABLE kiosks (
    outlet_id INT NOT NULL,
    branch_id INT NOT NULL,
    PRIMARY KEY (outlet_id, branch_id),
    CONSTRAINT fk_kiosk_outlet FOREIGN KEY (outlet_id)
        REFERENCES outlets(id) ON DELETE CASCADE,
    CONSTRAINT fk_kiosk_branch FOREIGN KEY (branch_id)
        REFERENCES branches(outlet_id) ON DELETE CASCADE
);

-- Фотомагазины
CREATE TABLE photo_stores (
    outlet_id INT NOT NULL PRIMARY KEY,
    CONSTRAINT fk_photo_store_outlet FOREIGN KEY (outlet_id)
        REFERENCES outlets(id) ON DELETE CASCADE
);

-- Типы услуг
CREATE TABLE service_types (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE,
    price NUMERIC(10, 2) NOT NULL CHECK (price > 0)
);

-- Типы услуг тороговых точек
CREATE TABLE service_types_outlets (
    id SERIAL PRIMARY KEY,
    service_type_id INT NOT NULL,
    outlet_type_id INT NOT NULL,
    CONSTRAINT fk_sto_service_type FOREIGN KEY (service_type_id)
        REFERENCES service_types(id) ON DELETE CASCADE,
    CONSTRAINT fk_sto_outlet_type FOREIGN KEY (outlet_type_id)
        REFERENCES outlet_types(id) ON DELETE CASCADE
);

-- Фирмы
CREATE TABLE firms (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE
);

-- Товары
CREATE TABLE items (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    price NUMERIC(10, 2) NOT NULL CHECK (price > 0),
    firm_id INT,
    CONSTRAINT fk_item_firm FOREIGN KEY (firm_id)
        REFERENCES firms(id) ON DELETE SET NULL
);

-- Клиенты
CREATE TABLE clients (
    id SERIAL PRIMARY KEY,
    full_name VARCHAR(255) NOT NULL,
    is_professional BOOLEAN NOT NULL,
    discount NUMERIC(5, 2) NOT NULL
);

-- Заказы
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    accept_time TIMESTAMP NOT NULL DEFAULT NOW(),
    overall_price NUMERIC(10, 2) NOT NULL,
    is_urgent BOOLEAN NOT NULL,
    outlet_id INT NOT NULL,
    client_id INT NOT NULL,
    CONSTRAINT fk_order_outlet FOREIGN KEY (outlet_id)
        REFERENCES outlets(id) ON DELETE CASCADE,
    CONSTRAINT fk_order_client FOREIGN KEY (client_id)
        REFERENCES clients(id) ON DELETE CASCADE
);

-- Заказы на печать
CREATE TABLE print_orders (
    id SERIAL PRIMARY KEY,
    order_id INT NOT NULL,
    print_discount_id INT NOT NULL,
    CONSTRAINT fk_print_order_order FOREIGN KEY (order_id)
        REFERENCES orders(id) ON DELETE CASCADE,
    CONSTRAINT fk_print_order_discount FOREIGN KEY (print_discount_id)
        REFERENCES print_discounts(id) ON DELETE CASCADE
);

-- Хранилища
CREATE TABLE storages (
    id SERIAL PRIMARY KEY,
    capacity INT NOT NULL,
    outlet_id INT NOT NULL,
    CONSTRAINT fk_storage_outlet FOREIGN KEY (outlet_id)
        REFERENCES outlets(id) ON DELETE CASCADE
);

-- Заказы услуг
CREATE TABLE service_orders (
    id SERIAL PRIMARY KEY,
    count INT NOT NULL,
    order_id INT NOT NULL,
    service_type_id INT NOT NULL,
    CONSTRAINT fk_service_order_order FOREIGN KEY (order_id)
        REFERENCES orders(id) ON DELETE CASCADE,
    CONSTRAINT fk_service_order_service_type FOREIGN KEY (service_type_id)
        REFERENCES service_types(id) ON DELETE CASCADE
);

-- Заказы на проявку плёнок
CREATE TABLE film_development_orders (
    id SERIAL PRIMARY KEY,
    service_order_id INT NOT NULL,
    code VARCHAR(255) NOT NULL,
    CONSTRAINT fk_film_dev_service_order FOREIGN KEY (service_order_id)
        REFERENCES service_orders(id) ON DELETE CASCADE
);

-- Плёнки
CREATE TABLE films (
    id SERIAL PRIMARY KEY,
    code VARCHAR(255) NOT NULL,
    service_order_id INT NOT NULL,
    CONSTRAINT fk_film_service_order FOREIGN KEY (service_order_id)
        REFERENCES service_orders(id) ON DELETE CASCADE
);

-- Поставщики
CREATE TABLE vendors (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL
);

-- Товары поставщика
CREATE TABLE vendor_items (
    id SERIAL PRIMARY KEY,
    price NUMERIC(10, 2) NOT NULL,
    quantity INT NOT NULL,
    vendor_id INT NOT NULL,
    item_id INT NOT NULL,
    CONSTRAINT fk_vendor_item_vendor FOREIGN KEY (vendor_id)
        REFERENCES vendors(id) ON DELETE CASCADE,
    CONSTRAINT fk_vendor_item_item FOREIGN KEY (item_id)
        REFERENCES items(id) ON DELETE CASCADE
);

-- Скидки на печать
CREATE TABLE print_discounts (
    id SERIAL PRIMARY KEY,
    photo_amount INT NOT NULL,
    discount NUMERIC(5, 2) NOT NULL
);

-- Типы бумаг
CREATE TABLE paper_types (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL
);

-- Форматы бумаг
CREATE TABLE paper_sizes (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL
);

-- Цены на печать
CREATE TABLE print_prices (
    id SERIAL PRIMARY KEY,
    price NUMERIC(10, 2) NOT NULL,
    paper_size_id INT NOT NULL,
    paper_type_id INT NOT NULL,
    CONSTRAINT fk_print_price_paper_size FOREIGN KEY (paper_size_id)
        REFERENCES paper_sizes(id),
    CONSTRAINT fk_print_price_paper_type FOREIGN KEY (paper_type_id)
        REFERENCES paper_types(id)
);

-- Кадры
CREATE TABLE frames (
    id SERIAL PRIMARY KEY,
    amount INT NOT NULL,
    frame_number INT NOT NULL,
    print_order_id INT NOT NULL,
    print_price_id INT NOT NULL,
    CONSTRAINT fk_frame_print_order FOREIGN KEY (print_order_id)
        REFERENCES print_orders(id),
    CONSTRAINT fk_frame_print_price FOREIGN KEY (print_price_id)
        REFERENCES print_prices(id)
);

-- Доставки
CREATE TABLE deliveries (
    id SERIAL PRIMARY KEY,
    date DATE NOT NULL,
    storage_id INT NOT NULL,
    vendor_id INT NOT NULL,
    CONSTRAINT fk_delivery_storage FOREIGN KEY (storage_id)
        REFERENCES storages(id),
    CONSTRAINT fk_delivery_vendor FOREIGN KEY (vendor_id)
        REFERENCES vendors(id)
);

-- Товары доставки
CREATE TABLE delivery_items (
    id SERIAL PRIMARY KEY,
    price NUMERIC(10, 2) NOT NULL,
    delivery_id INT NOT NULL,
    item_id INT NOT NULL,
    CONSTRAINT fk_delivery_item_delivery FOREIGN KEY (delivery_id)
        REFERENCES deliveries(id),
    CONSTRAINT fk_delivery_item_item FOREIGN KEY (item_id)
        REFERENCES items(id)
);

-- Товары необходимые для типов услуг
CREATE TABLE service_types_needed_items (
    item_id INT NOT NULL,
    service_type_id INT NOT NULL,
    count INT NOT NULL,
    PRIMARY KEY (item_id, service_type_id),
    CONSTRAINT fk_stni_item FOREIGN KEY (item_id)
        REFERENCES items(id),
    CONSTRAINT fk_stni_service_type FOREIGN KEY (service_type_id)
        REFERENCES service_types(id)
);

-- Товары хранилища
CREATE TABLE storage_items (
    id SERIAL PRIMARY KEY,
    quantity INT NOT NULL,
    item_id INT NOT NULL,
    storage_id INT NOT NULL,
    CONSTRAINT fk_storage_item_storage FOREIGN KEY (storage_id)
        REFERENCES storages(id),
    CONSTRAINT fk_storage_item_item FOREIGN KEY (item_id)
        REFERENCES items(id)
);