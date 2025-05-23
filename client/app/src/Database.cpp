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
                m_Connection = std::make_unique<pgfe::Connection>(pgfe::Connection_options{}
                                                                      .set(pgfe::Communication_mode::net)
                                                                      .set_hostname(m_Desc.HostName)
                                                                      .set_database(m_Desc.Database)
                                                                      .set_username(m_Desc.Username)
                                                                      .set_password(m_Desc.Password)
                                                                      .set_port(m_Desc.Port));
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
            return true;
        }

        return true;
    }

    std::optional<QueryResult> DatabaseConnection::Execute(const std::string& query) noexcept
    {
        std::optional<QueryResult> queryResult{std::nullopt};
        if (!TryConnectIfNotConnected()) return queryResult;

        try
        {
            m_Connection->execute(query);

            if (!m_Connection->row().is_empty())
            {
                QueryResult queryResultLocal{};

                // Column names
                {
                    const auto& rowDesc = m_Connection->row();
                    queryResultLocal.ColumnNames.resize(rowDesc.field_count());
                    for (uint32_t i{}; i < rowDesc.field_count(); ++i)
                        queryResultLocal.ColumnNames[i] = rowDesc.field_name(i);
                }

                // Rows
                while (m_Connection->completion())
                {
                    const auto& rowDesc = m_Connection->row();
                    if (rowDesc.is_empty()) continue;

                    auto& rowData = queryResultLocal.Rows.emplace_back();
                    rowData.resize(rowDesc.field_count());
                    for (uint32_t i{}; i < rowDesc.field_count(); ++i)
                        rowData[i] = rowDesc.field_name(i);
                }

                queryResult = std::move(queryResultLocal);
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(e.what());
            return queryResult;
        }

        return queryResult;
    }

}  // namespace nsudb
