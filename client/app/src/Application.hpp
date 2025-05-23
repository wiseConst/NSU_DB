#pragma once

#include <memory>

struct GLFWwindow;

namespace nsudb
{

    struct DatabaseConnection;
    struct Application final
    {
        Application() noexcept;
        ~Application() noexcept;

        void Run() noexcept;

      private:
        void Init() noexcept;
        void Shutdown() noexcept;

        std::unique_ptr<DatabaseConnection> m_DbConn;
        GLFWwindow* m_Window{nullptr};
    };

}  // namespace nsudb
