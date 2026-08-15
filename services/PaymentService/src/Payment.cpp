#include "Payment.h"

Payment::Payment(
    int id,
    int orderId,
    double amount,
    const std::string& paymentMethod,
    const std::string& transactionId,
    const std::string& status,
    const std::string& idempotencyKey,
    const std::string& provider,
    const std::string& providerPaymentId,
    const std::string& updatedAt)
    :
    m_id(id),
    m_orderId(orderId),
    m_amount(amount),
    m_paymentMethod(paymentMethod),
    m_transactionId(transactionId),
      m_status(status),
      m_idempotencyKey(idempotencyKey),
      m_provider(provider),
      m_providerPaymentId(providerPaymentId),
      m_updatedAt(updatedAt)
{
}

const std::string& Payment::getIdempotencyKey() const { return m_idempotencyKey; }
const std::string& Payment::getProvider() const { return m_provider; }
const std::string& Payment::getProviderPaymentId() const { return m_providerPaymentId; }
const std::string& Payment::getUpdatedAt() const { return m_updatedAt; }

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
