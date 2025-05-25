WITH filtered_orders AS (
    SELECT o.*, so.service_type_id, so.count, so.id AS service_order_id
    FROM orders o
    LEFT JOIN service_orders so ON o.id = so.order_id
    WHERE o.accept_time BETWEEN '2024-05-20 10:00:00' AND '2024-05-22 16:45:00'
      AND o.outlet_id IN (1, 3)
)
SELECT
    fo.service_type_id,
    st.name AS service_name,
    fo.is_urgent,
    COUNT(DISTINCT fo.id) AS orders_count
FROM filtered_orders fo
LEFT JOIN service_types st ON fo.service_type_id = st.id
GROUP BY fo.service_type_id, st.name, fo.is_urgent
ORDER BY st.name, fo.is_urgent;