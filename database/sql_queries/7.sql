SELECT DISTINCT
    v.id AS vendor_id,
    v.name AS vendor_name,
    i.id AS item_id,
    i.name AS item_name,
    di.quantity,
    di.price,
    d.date
FROM
    vendors v
JOIN deliveries d ON d.vendor_id = v.id
JOIN delivery_items di ON di.delivery_id = d.id
JOIN items i ON i.id = di.item_id
WHERE
    d.date BETWEEN '2024-01-01 10:00:00'::timestamp AND '2024-12-30 16:45:00'::timestamp
    AND di.quantity >= 2
ORDER BY v.id;
