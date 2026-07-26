import pytest
import os

@pytest.fixture(scope="session")
def gateway_url():
    return os.getenv("SMARTBOX_GATEWAY_URL", "http://localhost:8080")
