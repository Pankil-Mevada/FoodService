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

    def test_partner_portal_separates_navigation_and_draft_storage(self):
        html = (ROOT / "frontend" / "partner.html").read_text(encoding="utf-8")
        script = (ROOT / "frontend" / "partner.js").read_text(encoding="utf-8")
        for panel in ("overview", "restaurant", "menu", "orders", "team", "audit"):
            self.assertIn(f'data-panel="{panel}"', html)
        self.assertIn("plated_partner_draft_v1", script)
        self.assertNotIn("FoodServiceSecretKey", html + script)

    def test_partner_preview_does_not_claim_server_authorization(self):
        script = (ROOT / "frontend" / "partner.js").read_text(encoding="utf-8")
        self.assertIn("Browser storage is never partner authorization", script)
        self.assertIn("Publishing is blocked", script)


if __name__ == "__main__":
    unittest.main()
