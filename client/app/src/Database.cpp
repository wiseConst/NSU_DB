#include "Database.hpp"
#include <Logger.hpp>

#include <pgfe/pgfe.hpp>

namespace nsudb
{

    namespace pgfe = dmitigr::pgfe;

    DatabaseConnection::DatabaseConnection(const DatabaseDesc& databaseDesc) noexcept : m_Desc(databaseDesc) {}

    DatabaseConnection::~DatabaseConnection() noexcept
    {
        if (m_Connection && m_Connection->is_connected()) m_Connection->disconnect();
    }

    bool DatabaseConnection::TryConnectIfNotConnected() noexcept
    {
        if (!m_Connection)
        {
            try
            {
                static constexpr uint32_t s_ConnTimeoutSeconds = 5;

                const auto connectionOptions = pgfe::Connection_options{}
                                                   .set(pgfe::Communication_mode::net)
                                                   .set_hostname(m_Desc.HostName.c_str())
                                                   .set_database(m_Desc.Database.c_str())
                                                   .set_username(m_Desc.Username.c_str())
                                                   .set_password(m_Desc.Password.c_str())
                                                   .set_connect_timeout(std::chrono::seconds(s_ConnTimeoutSeconds))
                                                   .set_port(m_Desc.Port);
                m_Connection = std::make_unique<pgfe::Connection>(connectionOptions);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR(e.what());
                return false;
            }
        }

        if (!m_Connection->is_connected())
        {
            try
            {
                m_Connection->connect();
            }
            catch (const std::exception& e)
            {
                LOG_ERROR(e.what());
                return false;
            }

            LOG_TRACE("Connected to DB - {}, as {}", m_Desc.Database, m_Desc.Username);
            return true;
        }

        return true;
    }

    std::optional<QueryResult> DatabaseConnection::Execute(const std::string& query) noexcept
    {
        std::optional<QueryResult> queryResult{std::nullopt};
        if (!TryConnectIfNotConnected() || query.empty()) return queryResult;

        try
        {
            LOG_TRACE("Database: {}, executing query: {}", m_Desc.Database, query);

            QueryResult queryResultLocal{};
            bool bColumnsInitialized = false;

            m_Connection->execute(
                [&](auto&& row)
                {
                    if (!bColumnsInitialized)
                    {
                        queryResultLocal.ColumnNames.resize(row.field_count());
                        for (std::size_t i{}; i < row.field_count(); ++i)
                            queryResultLocal.ColumnNames[i] = row.field_name(i);

                        bColumnsInitialized = true;
                    }

                    auto& rowData = queryResultLocal.Rows.emplace_back();
                    rowData.resize(row.field_count());
                    for (std::size_t i{}; i < row.field_count(); ++i)
                    {
                        if (!row[i].is_empty())
                            rowData[i] = std::string((char*)row[i].bytes());
                        else
                            rowData[i] = "NULL";
                    }
                },
                query);

            queryResult = std::move(queryResultLocal);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(e.what());
        }

        return queryResult;
    }

}  // namespace nsudb
