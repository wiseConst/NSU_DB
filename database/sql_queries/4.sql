CREATE OR REPLACE VIEW filtered_orders AS
SELECT o.*
FROM orders o
WHERE o.accept_time BETWEEN '2024-05-20 10:00:00'::timestamp AND '2024-05-22 16:45:00'::timestamp
  AND (o.outlet_id = 1 OR o.outlet_id = 3);

CREATE OR REPLACE VIEW service_order_sums AS
SELECT o.is_urgent, so.service_type_id, SUM(o.overall_price) AS revenue
FROM filtered_orders o
JOIN service_orders so ON o.id = so.order_id
GROUP BY o.is_urgent, so.service_type_id;

SELECT
    sos.service_type_id,
    st.name AS service_name,
    sos.is_urgent,
    sos.revenue
FROM service_order_sums sos
JOIN service_types st ON sos.service_type_id = st.id
ORDER BY st.name, sos.is_urgent;
