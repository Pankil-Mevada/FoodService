import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class PortalContractTests(unittest.TestCase):
    def test_customer_portal_keeps_identity_and_address_entry_points(self):
        html = (ROOT / "frontend" / "index.html").read_text(encoding="utf-8")
        script = (ROOT / "frontend" / "app.js").read_text(encoding="utf-8")
        self.assertIn('id="profile-open"', html)
        self.assertIn('id="manage-addresses"', html)
        self.assertIn("plated_profile_user_", script)
        self.assertIn("Authorization", script)

    def test_partner_portal_uses_gateway_backed_identity_and_resources(self):
        html = (ROOT / "frontend" / "partner.html").read_text(encoding="utf-8")
        script = (ROOT / "frontend" / "partner.js").read_text(encoding="utf-8")
        for panel in ("overview", "restaurant", "menu", "orders", "team", "audit"):
            self.assertIn(f'data-panel="{panel}"', html)
        self.assertIn("/login", script)
        self.assertIn("/register", script)
        self.assertIn("/partner/restaurants", script)
        self.assertIn("Authorization", script)
        self.assertIn("plated_partner_token", script)
        self.assertNotIn("localStorage.getItem('plated_token')", script)
        self.assertIn('data-auth-mode="register"', html)
        self.assertNotIn("Create an account on the customer site", html)
        self.assertNotIn("plated_partner_draft_v1", script)
        self.assertNotIn("localhost:8081", html + script)
        self.assertNotIn("FoodServiceSecretKey", html + script)

    def test_partner_submission_remains_hidden_until_independent_review(self):
        html = (ROOT / "frontend" / "partner.html").read_text(encoding="utf-8")
        script = (ROOT / "frontend" / "partner.js").read_text(encoding="utf-8")
        controller = (ROOT / "services" / "RestaurantService" / "src" / "PartnerController.cpp").read_text(encoding="utf-8")
        self.assertIn("Submit for review", html)
        self.assertIn("PENDING_REVIEW", controller)
        self.assertIn("still hidden from customers", script)
        self.assertNotIn('body["status"]="APPROVED"', controller)


if __name__ == "__main__":
    unittest.main()
