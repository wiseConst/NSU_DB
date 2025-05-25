-- Перечень и общее число заказов на фотоработы по филиалам
SELECT
  b.outlet_id AS branch_outlet_id,
  COUNT(o.id) AS orders_count
FROM branches b
LEFT JOIN orders o ON o.outlet_id = b.outlet_id
  AND o.accept_time BETWEEN '2024-05-20 10:00:00'::timestamp AND '2024-05-22 16:45:00'::timestamp
GROUP BY b.outlet_id
ORDER BY b.outlet_id;


-- Перечень и общее число заказов по киоскам приема заказов
SELECT
  k.outlet_id AS kiosk_outlet_id,
  COUNT(o.id) AS orders_count
FROM kiosks k
LEFT JOIN orders o ON o.outlet_id = k.outlet_id
  AND o.accept_time BETWEEN '2024-05-20 10:00:00'::timestamp AND '2024-05-22 16:45:00'::timestamp
GROUP BY k.outlet_id
ORDER BY k.outlet_id;


-- Общее число заказов по фотоцентру за период
SELECT
  COUNT(*) AS total_orders
FROM orders
WHERE accept_time BETWEEN '2024-05-20 10:00:00'::timestamp AND '2024-05-22 16:45:00'::timestamp;
