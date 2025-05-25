-- Филиалы
SELECT b.outlet_id, o.address, ot.name AS outlet_type
FROM branches b
JOIN outlets o ON b.outlet_id = o.id
JOIN outlet_types ot ON o.type_id = ot.id;

-- Киоски
SELECT k.outlet_id, o.address, ot.name AS outlet_type, k.branch_id
FROM kiosks k
JOIN outlets o ON k.outlet_id = o.id
JOIN outlet_types ot ON o.type_id = ot.id;

-- Все пункты приема заказов
SELECT o.id AS outlet_id, o.address, ot.name AS outlet_type
FROM outlets o
JOIN outlet_types ot ON o.type_id = ot.id;

-- Общее число пунктов приема заказов (по фотоцентру)
SELECT COUNT(*) AS total_order_points FROM outlets;