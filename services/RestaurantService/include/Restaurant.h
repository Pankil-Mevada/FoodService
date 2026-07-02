class Restaurant
{
public:


    Restaurant(
        int id,
        const std::string& name,
        const std::string& address,
        const std::string& phone,
        double rating);

    int getId() const;

    const std::string& getName() const;

    const std::string& getAddress() const;

    const std::string& getPhone() const;

    double getRating() const;

private:

    int m_id{0};

    std::string m_name;

    std::string m_address;

    std::string m_phone;

    double m_rating{0.0};
};
