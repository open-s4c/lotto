use std::env;
use std::path::PathBuf;

fn get_lotto_dir() -> PathBuf {
    let pkg_dir = PathBuf::from(
        env::var("CARGO_MANIFEST_DIR").expect("Are you using cargo to build this crate?"),
    );
    let mut ancestors = pkg_dir.ancestors();
    for _ in 0..3 {
        let _parent = ancestors.next().expect("Failed to find lotto root dir");
    }
    let lotto_root_dir = ancestors.next().expect("Failed to find lotto root dir");
    lotto_root_dir.canonicalize().unwrap()
}

fn main() {
    let lottodir = get_lotto_dir();
    let rustbuilddir = lottodir.join("build").join("plugins").join("rust");
    let libpath = rustbuilddir.join("liblotto_rust_testing.a");
    if !libpath.exists() {
        panic!("liblotto_rust_testing.a not found; Please enable LOTTO_RUST and build once");
    }
    println!("cargo:rustc-link-search=native={}", rustbuilddir.display());
    println!("cargo:rustc-link-lib=static:+whole-archive=lotto_rust_testing");
}
