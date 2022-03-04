import os
import subprocess
import sys

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


MIN_CPU_CORES = 2


def get_cpu_count():
    try:
        return len(os.sched_getaffinity(0))  # linux only
    except:
        pass

    try:
        return os.cpu_count()  # python 3.4+
    except:
        return 1  # default


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    user_options = build_ext.user_options + [
        ('target=', None, "specify the CMake target to build")
    ]

    def initialize_options(self):
        self.target = "sonata_python"
        super(CMakeBuild, self).initialize_options()

    def run(self):
        try:
            subprocess.check_output(["cmake", "--version"])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: "
                + ", ".join(e.name for e in self.extensions)
            )

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        return None


# nearly verbatim from how h5py handles is
install_requires = [
    # We only really aim to support NumPy & Python combinations for which
    # there are wheels on PyPI (e.g. NumPy >=1.17.3 for Python 3.8).
    "numpy>=1.17.3",
]

setup_requires = [
    "setuptools_scm",
]

with open('README.rst') as f:
    README = f.read()

setup(
    name="libsonata",
    description='SONATA files reader',
    author="Blue Brain Project, EPFL",
    long_description=README,
    long_description_content_type='text/x-rst',
    license="LGPLv3",
    url='https://github.com/BlueBrain/libsonata',
    classifiers=[
        "License :: OSI Approved :: GNU Lesser General Public License v3 (LGPLv3)",
    ],
    ext_modules=[CMakeExtension("libsonata._libsonata")],
    cmdclass={'build_ext': CMakeBuild,
              },
    zip_safe=False,
    setup_requires=setup_requires,
    install_requires=install_requires,
    extras_require={
        'docs': ['sphinx-bluebrain-theme'],
    },
    python_requires=">=3.8",
    use_scm_version={"local_scheme": "no-local-version",
                     },
    package_dir={"": "python"},
    packages=['libsonata',
              ],
)
