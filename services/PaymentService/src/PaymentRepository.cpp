#include "PaymentRepository.h"

#include <iostream>
#include <sqlite3.h>

PaymentRepository::PaymentRepository(Database& database)
    : m_database(database)
{
}

bool PaymentRepository::savePayment(const Payment& payment)
{
    const char* sql =
        "INSERT INTO payments(order_id,amount,payment_method,transaction_id,status)"
        " VALUES(?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << sqlite3_errmsg(m_database.connection()) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, payment.getOrderId());
    sqlite3_bind_double(stmt, 2, payment.getAmount());
    sqlite3_bind_text(stmt, 3,
                      payment.getPaymentMethod().c_str(),
                      -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4,
                      payment.getTransactionId().c_str(),
                      -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5,
                      payment.getStatus().c_str(),
                      -1,
                      SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<Payment> PaymentRepository::getAllPayments()
{
    std::vector<Payment> payments;

    const char* sql =
        "SELECT id,order_id,amount,payment_method,transaction_id,status "
        "FROM payments;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return payments;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        payments.emplace_back(
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1),
            sqlite3_column_double(stmt, 2),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
    }

    sqlite3_finalize(stmt);

    return payments;
}

std::optional<Payment> PaymentRepository::getPaymentById(int id)
{
    const char* sql =
        "SELECT id,order_id,amount,payment_method,transaction_id,status "
        "FROM payments WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Payment payment(
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1),
            sqlite3_column_double(stmt, 2),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));

        sqlite3_finalize(stmt);

        return payment;
    }

    sqlite3_finalize(stmt);

    return std::nullopt;
}

bool PaymentRepository::updatePayment(const Payment& payment)
{
    const char* sql =
        "UPDATE payments "
        "SET order_id=?,amount=?,payment_method=?,transaction_id=?,status=? "
        "WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, payment.getOrderId());
    sqlite3_bind_double(stmt, 2, payment.getAmount());
    sqlite3_bind_text(stmt, 3,
                      payment.getPaymentMethod().c_str(),
                      -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4,
                      payment.getTransactionId().c_str(),
                      -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5,
                      payment.getStatus().c_str(),
                      -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, payment.getId());

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        return false;
    }

    return sqlite3_changes(m_database.connection()) > 0;
}

bool PaymentRepository::deletePayment(int id)
{
    const char* sql =
        "DELETE FROM payments WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        return false;
    }

    return sqlite3_changes(m_database.connection()) > 0;
}
