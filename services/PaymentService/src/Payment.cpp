#include "Payment.h"

Payment::Payment(
    int id,
    int orderId,
    double amount,
    const std::string& paymentMethod,
    const std::string& transactionId,
    const std::string& status)
    :
    m_id(id),
    m_orderId(orderId),
    m_amount(amount),
    m_paymentMethod(paymentMethod),
    m_transactionId(transactionId),
    m_status(status)
{
}

int Payment::getId() const
{
    return m_id;
}

int Payment::getOrderId() const
{
    return m_orderId;
}

double Payment::getAmount() const
{
    return m_amount;
}

const std::string& Payment::getPaymentMethod() const
{
    return m_paymentMethod;
}

const std::string& Payment::getTransactionId() const
{
    return m_transactionId;
}

const std::string& Payment::getStatus() const
{
    return m_status;
}
