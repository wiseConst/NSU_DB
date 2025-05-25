SELECT
    COALESCE(SUM(stni.count * i.price * so.count), 0) AS total_revenue
FROM
    orders o
JOIN service_orders so ON so.order_id = o.id
JOIN service_types_needed_items stni ON stni.service_type_id = so.service_type_id
JOIN items i ON i.id = stni.item_id
WHERE
    o.accept_time BETWEEN '2024-05-20 10:00:00'::timestamp
                     AND '2024-05-22 16:45:00'::timestamp
    AND o.outlet_id = 1;
