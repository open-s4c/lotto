use anyhow::{Context, Result};
use cmake_package::find_package;
use std::{env, path::PathBuf};

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

fn main() -> Result<()> {
    let lotto_dir = get_lotto_dir();
    println!("Lotto dir is {}", lotto_dir.display());

    let lotto_user_include_dir = get_lotto_dir().join("include");
    assert!(
        lotto_user_include_dir.exists(),
        "Could not find lotto user include dir at {:?}",
        lotto_user_include_dir
    );

    let clang_user_include_arg = format!("-I{}", lotto_user_include_dir.display());
    println!("Lotto user include dir is {}", clang_user_include_arg);

    let lotto_include_dir = get_lotto_dir().join("src").join("include");
    assert!(
        lotto_include_dir.exists(),
        "Could not find lotto include dir at {:?}",
        lotto_include_dir
    );

    let clang_include_arg = format!("-I{}", lotto_include_dir.display());
    println!("Lotto include dir is {}", clang_include_arg);

    let lotto_build_include_dir = get_lotto_dir().join("build").join("include");
    assert!(
        lotto_build_include_dir.exists(),
        "Could not find lotto build include dir at {:?}",
        lotto_build_include_dir
    );

    let clang_build_include_arg = format!("-I{}", lotto_build_include_dir.display());
    println!("Lotto build include dir is {}", clang_build_include_arg);

    let lotto_build_gen_include_dir = get_lotto_dir().join("build").join("generated");
    assert!(
        lotto_build_gen_include_dir.exists(),
        "Could not find lotto build generated include dir at {:?}",
        lotto_build_gen_include_dir
    );

    let clang_build_gen_include_arg = format!("-I{}", lotto_build_gen_include_dir.display());
    println!(
        "Lotto build gen include dir is {}",
        clang_build_gen_include_arg
    );

    let libvsync_package = find_package("libvsync")
        .prefix_paths(vec![get_lotto_dir()
            .join("deps")
            .join("dice")
            .join("deps")
            .join("libvsync")])
        .find()
        .context("could not find libvsync package")?;
    let vsync_target = libvsync_package
        .target("libvsync::vsync")
        .ok_or_else(|| anyhow::anyhow!("Could not find target 'libvsync::vsync'"))?;
    let libvsync_include_paths = vsync_target.include_directories;
    println!("Libsync include dirs are {:?}", libvsync_include_paths);

    let dice_include_dir = get_lotto_dir().join("deps").join("dice").join("include");
    assert!(
        dice_include_dir.exists(),
        "Could not find Dice include dir at {:?}",
        dice_include_dir
    );
    let dice_include_arg = format!("-I{}", dice_include_dir.display());
    println!("Dice include dir is {}", dice_include_arg);

    let mut modules_include_dirs: Vec<_> = vec!["termination", "ichpt"]
        .into_iter()
        .map(|m| get_lotto_dir().join("modules").join(m).join("include"))
        .collect();
    let rusty_include_dir = get_lotto_dir()
        .join("build")
        .join("modules")
        .join("rusty")
        .join("include");
    modules_include_dirs.push(rusty_include_dir);

    println!("Modules include dir is {:?}", modules_include_dirs);
    modules_include_dirs
        .iter()
        .for_each(|m| assert!(m.exists(), "Could not find Dice include dir at {:?}", m));

    #[allow(unused_mut)]
    let mut builder = bindgen::Builder::default()
        .header("wrapper.h")
        .allowlist_file(".*lotto/.*")
        .allowlist_file(".*dice/.*")
        .newtype_enum("base_category")
        .rustified_enum("reason")
        .rustified_enum("slot")
        .rustified_enum("topic")
        .rustified_enum("enforece_mode")
        .rustified_enum("termination_mode")
        .rustified_enum("stable_address_method")
        .rustified_enum("subcmd_group")
        .rustified_enum("state_type")
        .rustified_enum("str_converter_type")
        .rustified_enum("record_granularity")
        .constified_enum_module("rmw_op")
        .bitfield_enum("record")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .clang_arg(clang_user_include_arg)
        .clang_arg(clang_include_arg)
        .clang_arg(clang_build_include_arg)
        .clang_arg(clang_build_gen_include_arg)
        .clang_arg(dice_include_arg)
        .clang_arg("-D_GNU_SOURCE");
    builder = libvsync_include_paths
        .into_iter()
        .map(|include_dir| format!("-I{}", include_dir))
        .fold(builder, |builder, include_arg| {
            builder.clang_arg(include_arg)
        });
    builder = modules_include_dirs
        .into_iter()
        .map(|include_dir| format!("-I{}", include_dir.display()))
        .fold(builder, |builder, include_arg| {
            builder.clang_arg(include_arg)
        });

    #[cfg(feature = "stable_address_map")]
    {
        builder = builder.clang_arg("-DLOTTO_STABLE_ADDRESS_MAP");
    }

    let bindings = builder
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");

    Ok(())
}
