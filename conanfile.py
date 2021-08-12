from conans import ConanFile

class ConanPackage(ConanFile):
    name = 'network-monitor'
    version = "0.1.0"

    generators = 'cmake_find_package'

    requires = [
        ('boost/1.76.0'),
        ("libcurl/7.73.0"),
        ('cpr/1.6.2'),
        ('openssl/1.1.1k'),
        ('nlohmann_json/3.9.1')
    ]

    default_options = (
        'boost:shared=False',
    )