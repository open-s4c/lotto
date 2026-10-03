use std::collections::{BTreeMap, BTreeSet};

/// Evidence belongs to a clock, not to a single binary-search pass.
#[derive(Default)]
pub struct Samples(BTreeMap<u64, u64>);

impl Samples {
    pub fn probe<E>(
        &mut self,
        clock: u64,
        rounds: u64,
        pred: impl FnOnce() -> Result<Option<bool>, E>,
    ) -> Result<Option<bool>, E> {
        // Match the fast search's existing `confidence <= rounds` limit.
        if self.0.get(&clock).is_some_and(|count| *count > rounds) {
            return Ok(Some(true));
        }
        let result = pred()?;
        match result {
            Some(true) => {
                let count = self.0.entry(clock).or_default();
                *count = count.saturating_add(1);
            }
            Some(false) => {
                self.0.remove(&clock);
            }
            None => {}
        }
        Ok(result)
    }
}

/// Find the first true clock with a known true upper bound.
/// `None` skips a probe for this pass without excluding it as a result.
pub fn binary_search<E>(
    mut lower: u64,
    mut upper: u64,
    mut pred: impl FnMut(u64) -> Result<Option<bool>, E>,
) -> Result<u64, E> {
    while lower < upper {
        let mut skipped = BTreeSet::new();
        while lower < upper {
            let middle = lower + (upper - lower) / 2;
            let Some(clock) = (middle..upper)
                .chain((lower..middle).rev())
                .find(|clock| !skipped.contains(clock))
            else {
                break;
            };
            match pred(clock)? {
                Some(true) => upper = clock,
                Some(false) => lower = clock + 1,
                None => {
                    skipped.insert(clock);
                }
            }
        }
    }
    Ok(upper)
}

#[cfg(test)]
mod tests {
    use super::{binary_search, Samples};

    #[test]
    fn samples_survive_passes_and_discards() {
        let mut samples = Samples::default();
        // The first pass observes clock 4, but returns candidate 2.
        assert_eq!(
            binary_search(0, 8, |clock| {
                samples.probe(clock, 1, || Ok::<_, ()>(Some(clock >= 2)))
            }),
            Ok(2)
        );
        // A later pass raises the candidate to 4; reuse its earlier sample.
        assert_eq!(
            binary_search(2, 8, |clock| {
                samples.probe(clock, 1, || Ok::<_, ()>(Some(clock >= 4)))
            }),
            Ok(4)
        );
        assert_eq!(
            samples.probe(4, 1, || panic!("already sampled twice")),
            Ok::<_, ()>(Some(true))
        );

        assert_eq!(
            samples.probe(6, 1, || Ok::<_, ()>(Some(true))),
            Ok(Some(true))
        );
        assert_eq!(samples.probe(6, 1, || Ok::<_, ()>(None)), Ok(None));
        assert_eq!(samples.0.get(&6), Some(&1));
        assert_eq!(
            samples.probe(6, 1, || Ok::<_, ()>(Some(false))),
            Ok(Some(false))
        );
        assert_eq!(
            samples.probe(6, 1, || Ok::<_, ()>(Some(true))),
            Ok(Some(true))
        );
        assert_eq!(
            samples.probe(6, 1, || Err::<Option<bool>, _>("still needs a sample")),
            Err("still needs a sample")
        );
        assert_eq!(
            samples.probe(6, 1, || Ok::<_, ()>(Some(true))),
            Ok(Some(true))
        );
        assert_eq!(
            samples.probe(6, 1, || panic!("already sampled twice")),
            Ok::<_, ()>(Some(true))
        );
    }

    #[test]
    fn skips_are_temporary_and_do_not_remove_possible_results() {
        let mut visited = Vec::new();
        let found = binary_search(0, 8, |clock| {
            visited.push(clock);
            Ok::<_, ()>(if visited.len() == 1 {
                None
            } else {
                Some(clock >= 5)
            })
        });
        assert_eq!(found, Ok(5));
        assert_eq!(visited, [4, 5, 2, 3, 4]);

        for boundary in 0..=8 {
            let mut visits = [0; 9];
            assert_eq!(
                binary_search(0, 8, |clock| {
                    visits[clock as usize] += 1;
                    Ok::<_, ()>((visits[clock as usize] > 1).then_some(clock >= boundary))
                }),
                Ok(boundary)
            );
        }
        assert_eq!(
            binary_search(0, u64::MAX, |clock| Ok::<_, ()>(Some(
                clock >= u64::MAX - 1
            ))),
            Ok(u64::MAX - 1)
        );
        assert_eq!(
            binary_search(7, 7, |_| Err::<Option<bool>, _>("unused")),
            Ok(7)
        );
        assert_eq!(
            binary_search(0, 8, |_| Err::<Option<bool>, _>("stop")),
            Err("stop")
        );
    }
}
