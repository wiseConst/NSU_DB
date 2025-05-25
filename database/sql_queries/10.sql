-- Перечень фототоваров и фирм, производящих их,
-- которые пользуются наибольшим спросом по фотоцентру и филиалу с id = 1

WITH orders_in_branch AS (
    SELECT o.id
    FROM orders o
    WHERE o.outlet_id = 1
),

service_orders_in_branch AS (
    SELECT so.*
    FROM service_orders so
    JOIN orders_in_branch ob ON so.order_id = ob.id
),

items_demand_in_branch AS (
    SELECT
        stni.item_id,
        SUM(stni.count * so.count) AS total_quantity
    FROM service_orders_in_branch so
    JOIN service_types_needed_items stni ON so.service_type_id = stni.service_type_id
    GROUP BY stni.item_id
),

items_demand_overall AS (
    SELECT
        stni.item_id,
        SUM(stni.count * so.count) AS total_quantity
    FROM service_orders so
    JOIN service_types_needed_items stni ON so.service_type_id = stni.service_type_id
    GROUP BY stni.item_id
)

SELECT
    i.id AS item_id,
    i.name AS item_name,
    f.name AS firm_name,
    COALESCE(d_branch.total_quantity, 0) AS demand_in_branch,
    COALESCE(d_overall.total_quantity, 0) AS demand_overall
FROM items i
LEFT JOIN firms f ON i.firm_id = f.id
LEFT JOIN items_demand_in_branch d_branch ON i.id = d_branch.item_id
LEFT JOIN items_demand_overall d_overall ON i.id = d_overall.item_id
WHERE COALESCE(d_branch.total_quantity, 0) > 0 OR COALESCE(d_overall.total_quantity, 0) > 0
ORDER BY demand_overall DESC, demand_in_branch DESC;
