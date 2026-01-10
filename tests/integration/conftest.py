import pytest
import os
import glob
from tests.integration.engine.director import OmnisDirector
from tests.integration.engine.observer import OmnisObserver

def get_product_manifests():
    # Find all omnis_manifest.json in build/products/*/generated/
    build_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../build"))
    pattern = os.path.join(build_dir, "products/*/generated/omnis_manifest.json")
    return glob.glob(pattern)

@pytest.fixture(scope="function", params=get_product_manifests())
def director(request):
    manifest_path = request.param
    product_dir = os.path.dirname(manifest_path)
    d = OmnisDirector(product_dir)
    d.start_edgec()
    yield d
    d.stop_all()

@pytest.fixture(scope="function")
def observer(director):
    return OmnisObserver(director.rest_port, director.db_path)
