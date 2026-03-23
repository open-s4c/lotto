use lotto_sys as raw;

pub fn effective_event_type(ctx: &raw::capture_point) -> u32 {
    ctx.src_type as u32
}
