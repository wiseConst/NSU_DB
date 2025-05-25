WITH filtered_orders AS (
    SELECT o.id
    FROM orders o
    WHERE o.accept_time BETWEEN '2024-05-20 10:00:00'::timestamp 
                            AND '2024-05-22 16:45:00'::timestamp
    AND o.outlet_id = 1
),

service_orders_filtered AS (
    SELECT so.*
    FROM service_orders so
    JOIN filtered_orders fo ON so.order_id = fo.id
),

items_sold AS (
    SELECT 
        stni.item_id,
        SUM(stni.count * so.count) AS total_quantity
    FROM service_orders_filtered so
    JOIN service_types_needed_items stni ON so.service_type_id = stni.service_type_id
    GROUP BY stni.item_id
)

SELECT 
    i.id AS item_id,
    i.name AS item_name,
    f.name AS firm_name,
    COALESCE(items_sold.total_quantity, 0) AS quantity_sold
FROM items i
LEFT JOIN firms f ON i.firm_id = f.id
LEFT JOIN items_sold ON i.id = items_sold.item_id
WHERE items_sold.total_quantity IS NOT NULL
ORDER BY quantity_sold DESC;
