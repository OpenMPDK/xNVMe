# -*- coding: utf-8 -*-
#
# xNVMe documentation build configuration file, created by sphinx-quickstart
import os

from xnvme_ver import xnvme_ver

on_rtd = os.environ.get("READTHEDOCS", None) == "True"

extensions = [
    "sphinx.ext.todo",
    "sphinx.ext.imgmath",
    #    'sphinxcontrib.bibtex',
    "sphinxcontrib.jquery",
    "sphinx_rtd_theme",
    "breathe",
]

exclude_patterns = ["autogen"]

source_suffix = ".rst"
master_doc = "index"
project = "xNVMe"
copyright = "2024, xNVMe"
version = xnvme_ver(os.path.join("..", "..", "meson.build"))
release = xnvme_ver(os.path.join("..", "..", "meson.build"))

pygments_style = "sphinx"

if not on_rtd:
    html_theme = "sphinx_rtd_theme"

html_theme_options = {"analytics_id": "UA-159785887-1"}
html_css_files = ["theme_overrides.css"]
html_static_path = [os.path.join("..", "_static")]

# Output file base name for HTML help builder.
htmlhelp_basename = "xnvmedoc"

breathe_projects = {project: os.path.join("builddir", "doxy", "xml")}
breathe_default_project = project
breathe_domain_by_extension = {"h": "c"}
