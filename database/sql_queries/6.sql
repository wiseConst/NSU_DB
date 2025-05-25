-- филиал (outlet_id = 1)
SELECT
    o.is_urgent,
    COUNT(f.id) AS total_films
FROM films f
JOIN service_orders so ON f.service_order_id = so.id
JOIN orders o ON o.id = so.order_id
WHERE
    o.accept_time BETWEEN '2024-01-01 10:00:00'::timestamp AND '2024-12-30 16:45:00'::timestamp
    AND o.outlet_id IN (SELECT outlet_id FROM branches WHERE outlet_id = 1)
GROUP BY o.is_urgent;

-- По киоску (kiosk_outlet_id = 3)
SELECT
    o.is_urgent,
    COUNT(f.id) AS total_films
FROM films f
JOIN service_orders so ON f.service_order_id = so.id
JOIN orders o ON o.id = so.order_id
WHERE
    o.accept_time BETWEEN '2024-01-01 10:00:00'::timestamp AND '2024-12-30 16:45:00'::timestamp
    AND o.outlet_id IN (SELECT outlet_id FROM kiosks WHERE outlet_id = 3)
GROUP BY o.is_urgent;