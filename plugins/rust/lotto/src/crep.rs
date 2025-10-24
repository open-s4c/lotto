use lotto_sys as raw;

pub fn backup_make() {
    unsafe { raw::crep_backup_make() }
}

pub fn backup_restore() {
    unsafe { raw::crep_backup_restore() }
}

pub fn backup_clean() {
    unsafe { raw::crep_backup_clean() }
}

pub fn truncate(clk: raw::clk_t) {
    unsafe { raw::crep_truncate(clk) }
}
