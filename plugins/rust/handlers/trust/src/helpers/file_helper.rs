//! Implements helper functions to read and write to a file.
//!
use lotto::log::{debug, trace, warn};
use memmap;
use memmap::Mmap;
use std::fs;
use std::fs::File;
use std::io;
use std::io::BufRead;

pub fn write_to_file(file_name: &String, content: &String) {
    let result = fs::write(file_name, content);
    match result {
        Ok(_) => {
            debug!("Saved state to file.");
        }
        Err(e) => {
            panic!(
                "Unable to save state in TruSt handler due to error: {:?}",
                e
            );
        }
    }
}

pub fn read_single_lined_file(file_name: &String) -> Option<String> {
    let f: Result<File, io::Error> = File::open(file_name);
    if f.is_err() {
        return None;
    }
    debug!("Opening previous run file: {:?}", file_name);
    let f = f.unwrap();
    let mmap =
        unsafe { Mmap::map(&f).unwrap_or_else(|_| panic!("Error mapping file {}", &file_name)) };

    let reader: Box<dyn io::BufRead> = Box::new(&mmap[..]);

    let lines_vec: Vec<String> = reader.lines().map_while(Result::ok).collect();
    assert!(lines_vec.len() == 1);
    assert!(!lines_vec[0].is_empty());
    Some(lines_vec[0].clone())
}

pub fn delete_file(file_name: &String) {
    match fs::remove_file(file_name) {
        Ok(_) => {
            trace!("File {:?} deleted.", &file_name);
        }
        Err(err) => {
            warn!(
                "Lotto TruSt file {:?} not deleted due to error: {:?}",
                &file_name, err
            );
        }
    }
}
