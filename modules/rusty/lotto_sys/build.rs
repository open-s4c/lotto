use anyhow::{anyhow, Context, Result};
use cmake_package::find_package;
use std::{env, fs, path::PathBuf};

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

fn get_lotto_build_dir() -> PathBuf {
    if let Ok(build_dir) = env::var("LOTTO_BUILD_DIR") {
        return PathBuf::from(build_dir).canonicalize().unwrap();
    }

    let out_dir =
        PathBuf::from(env::var("OUT_DIR").expect("Cargo did not provide OUT_DIR for lotto_sys"));
    for ancestor in out_dir.ancestors() {
        if ancestor.join("include").exists() && ancestor.join("generated").exists() {
            return ancestor.to_path_buf();
        }
    }

    get_lotto_dir().join("build")
}

fn get_rusty_module_slot(lotto_build_dir: &PathBuf) -> Result<i32> {
    let rusty_header = lotto_build_dir
        .join("modules")
        .join("rusty")
        .join("include")
        .join("lotto")
        .join("modules")
        .join("rusty")
        .join("rusty.h");
    let content = fs::read_to_string(&rusty_header)
        .with_context(|| format!("could not read {}", rusty_header.display()))?;
    let define = content
        .lines()
        .find_map(|line| line.strip_prefix("#define MODULE_SLOT "))
        .ok_or_else(|| anyhow!("could not find MODULE_SLOT in {}", rusty_header.display()))?;
    define.trim().parse::<i32>().with_context(|| {
        format!(
            "could not parse MODULE_SLOT from {}",
            rusty_header.display()
        )
    })
}

fn collect_module_include_dirs(root: &PathBuf) -> Result<Vec<PathBuf>> {
    let modules_dir = root.join("modules");
    let mut include_dirs = Vec::new();
    if !modules_dir.exists() {
        return Ok(include_dirs);
    }

    for entry in fs::read_dir(&modules_dir)
        .with_context(|| format!("could not read {}", modules_dir.display()))?
    {
        let entry = entry?;
        let include_dir = entry.path().join("include");
        if include_dir.exists() {
            include_dirs.push(include_dir.canonicalize().unwrap_or(include_dir));
        }
    }

    Ok(include_dirs)
}

fn main() -> Result<()> {
    let lotto_dir = get_lotto_dir();
    let lotto_build_dir = get_lotto_build_dir();
    println!("Lotto dir is {}", lotto_dir.display());
    println!("Lotto build dir is {}", lotto_build_dir.display());

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

    let lotto_build_include_dir = lotto_build_dir.join("include");
    assert!(
        lotto_build_include_dir.exists(),
        "Could not find lotto build include dir at {:?}",
        lotto_build_include_dir
    );

    let clang_build_include_arg = format!("-I{}", lotto_build_include_dir.display());
    println!("Lotto build include dir is {}", clang_build_include_arg);

    let lotto_build_gen_include_dir = lotto_build_dir.join("generated");
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

    let mut modules_include_dirs = collect_module_include_dirs(&lotto_build_dir)?;
    modules_include_dirs.extend(collect_module_include_dirs(&get_lotto_dir())?);
    modules_include_dirs.sort();
    modules_include_dirs.dedup();

    println!("Modules include dir is {:?}", modules_include_dirs);
    modules_include_dirs
        .iter()
        .for_each(|m| assert!(m.exists(), "Could not find Dice include dir at {:?}", m));

    #[allow(unused_mut)]
    let mut builder = bindgen::Builder::default()
        .header("wrapper.h")
        .allowlist_file(".*lotto/.*")
        .allowlist_file(".*dice/.*")
        .rustified_enum("reason")
        .rustified_enum("slot")
        .rustified_enum("event")
        .rustified_enum("enforece_mode")
        .rustified_enum("terminate_mode")
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
        .clang_arg("-D_GNU_SOURCE")
        .clang_arg("-DLOGGER_PREFIX=\"lotto_sys\"");
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

    let rusty_module_slot = get_rusty_module_slot(&lotto_build_dir)?;
    fs::write(
        out_path.join("module_slot.rs"),
        format!("pub const LOTTO_RUSTY_MODULE_SLOT: i32 = {rusty_module_slot};\n"),
    )
    .context("could not write module_slot.rs")?;

    Ok(())
}
