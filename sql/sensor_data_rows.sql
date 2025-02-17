CREATE OR REPLACE FUNCTION get_avg_by_email(email_param TEXT)
RETURNS TABLE (email TEXT, dataType TEXT, avg_data BIGINT) AS $$
BEGIN
    RETURN QUERY 
    SELECT sensor_data.email, dataType, AVG(sensor_data.data)::BIGINT AS AVG_temperature
    FROM sensor_data
    WHERE sensor_data.email = email_param
    GROUP BY sensor_data.email, dataType;
END;
$$ LANGUAGE plpgsql;