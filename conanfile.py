from conans import ConanFile

class ConanPackage(ConanFile):
    name = 'network-monitor'
    version = "0.1.0"

    generators = 'cmake_find_package'

    requires = [
        ('boost/1.76.0'),
        ('openssl/1.1.1k'),
    ]

    default_options = (
        'boost:shared=False',
    )