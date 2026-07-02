class JwtManager
{
public:
	static std::string generateToken(
        int userId,
        const std::string& email);

    static bool verifyToken(
        const std::string& token);

    static jwt::decoded_jwt<> decodeToken(
        const std::string& token);
};
