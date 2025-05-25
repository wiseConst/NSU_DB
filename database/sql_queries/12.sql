-- Все рабочие места (без фильтра по профилю)
SELECT o.id AS outlet_id, o.address, ot.name AS outlet_type
FROM outlets o
JOIN outlet_types ot ON o.type_id = ot.id
ORDER BY o.id;

-- Рабочие места с профилем 'kiosk'
SELECT o.id AS outlet_id, o.address, ot.name AS outlet_type
FROM outlets o
JOIN outlet_types ot ON o.type_id = ot.id
WHERE ot.name = 'kiosk'
ORDER BY o.id;
