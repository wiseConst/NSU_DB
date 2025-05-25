SELECT DISTINCT
    c.id AS client_id,
    c.full_name,
    c.discount,
    o.id AS order_id,
    o.overall_price,
    o.outlet_id
FROM
    clients c
JOIN orders o ON o.client_id = c.id
WHERE
    c.discount > 0
    AND o.overall_price >= 5  --  если такой порог
    AND o.outlet_id = 3       --  если нужен только 1 филиал
ORDER BY c.id 