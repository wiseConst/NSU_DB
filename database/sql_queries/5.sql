-- Запрос для подсчёта отпечатанных фотографий по филиалу (outlet_id из branches):
SELECT
    o.is_urgent,
    SUM(f.amount) AS total_printed_photos
FROM frames f
JOIN print_orders po ON f.print_order_id = po.id
JOIN orders o ON po.order_id = o.id
WHERE o.accept_time BETWEEN '2024-01-01 10:00:00'::timestamp
                      AND '2024-12-30 16:45:00'::timestamp
  AND o.outlet_id IN (SELECT outlet_id FROM branches)
GROUP BY o.is_urgent;

--Запрос для киоска (outlet_id из kiosks):
SELECT
    o.is_urgent,
    SUM(f.amount) AS total_printed_photos
FROM frames f
JOIN print_orders po ON f.print_order_id = po.id
JOIN orders o ON po.order_id = o.id
WHERE o.accept_time BETWEEN '2024-01-01 10:00:00'::timestamp
                      AND '2024-12-30 16:45:00'::timestamp
  AND o.outlet_id IN (SELECT outlet_id FROM kiosks)
GROUP BY o.is_urgent;

-- Запрос по всему фотоцентру (без фильтра):
SELECT
    o.is_urgent,
    SUM(f.amount) AS total_printed_photos
FROM frames f
JOIN print_orders po ON f.print_order_id = po.id
JOIN orders o ON po.order_id = o.id
WHERE o.accept_time BETWEEN '2024-01-01 10:00:00'::timestamp
                      AND '2024-12-30 16:45:00'::timestamp
GROUP BY o.is_urgent;