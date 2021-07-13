fn main() {
    let net_snmp = pkg_config::Config::new().probe("netsnmp").unwrap();

    cc::Build::new()
        .file("zs_snmp_support.c")
        .includes(net_snmp.include_paths)
        .warnings(true)
        .compile("libzs_snmp_support.a");

    for i in net_snmp.libs {
        println!("cargo:rustc-link-lib={}", i);
    }
}
