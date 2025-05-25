#pragma once

#include <memory>
#include <cstdint>

namespace dmitigr::pgfe
{
    class Connection;
}

namespace nsudb
{

    struct DatabaseDesc final
    {
        std::string HostName{"127.0.0.1"};
        std::string Database{};
        std::string Username{};
        std::string Password{};
        int_fast32_t Port{5432};
    };

    struct QueryResult final
    {
        std::vector<std::vector<std::string>> Rows;
        std::vector<std::string> ColumnNames;
    };

    struct DatabaseConnection final
    {
        DatabaseConnection(const DatabaseDesc& databaseDesc) noexcept;
        ~DatabaseConnection() noexcept;

        bool TryConnectIfNotConnected() noexcept;

        std::optional<QueryResult> Execute(const std::string& query) noexcept;

      private:
        DatabaseDesc m_Desc{};
        std::unique_ptr<dmitigr::pgfe::Connection> m_Connection{nullptr};
    };

}  // namespace nsudb
