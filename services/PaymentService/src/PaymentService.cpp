#include "PaymentService.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>

int PaymentService::m_transactionCounter = 1;

PaymentService::PaymentService(PaymentRepository& repository)
    : m_repository(repository)
{
}

bool PaymentService::createPayment(
    int orderId,
    double amount,
    const std::string& paymentMethod)
{
    Payment payment(
        0,
        orderId,
        amount,
        paymentMethod,
        generateTransactionId(),
        "SUCCESS");

    bool status = m_repository.savePayment(payment);

    if (!status)
    {
        return false;
    }

    bool notificationStatus =
        m_notificationClient.createNotification(
            1,
            "Payment Successful");

    if (!notificationStatus)
    {
        std::cout << "Notification failed" << std::endl;
    }

    return true;
}
std::vector<Payment> PaymentService::getAllPayments()
{
    return m_repository.getAllPayments();
}

std::optional<Payment> PaymentService::getPaymentById(int id)
{
    return m_repository.getPaymentById(id);
}

bool PaymentService::updatePayment(const Payment& payment)
{
    return m_repository.updatePayment(payment);
}

bool PaymentService::deletePayment(int id)
{
    return m_repository.deletePayment(id);
}

std::string PaymentService::generateTransactionId()
{
    std::time_t now = std::time(nullptr);

    std::tm* timeInfo = std::localtime(&now);

    std::ostringstream oss;

    oss << "TXN-";

    oss << (timeInfo->tm_year + 1900);

    oss << std::setw(2)
        << std::setfill('0')
        << (timeInfo->tm_mon + 1);

    oss << std::setw(2)
        << std::setfill('0')
        << timeInfo->tm_mday;

    oss << "-";

    oss << std::setw(6)
        << std::setfill('0')
        << m_transactionCounter++;

    return oss.str();
}
