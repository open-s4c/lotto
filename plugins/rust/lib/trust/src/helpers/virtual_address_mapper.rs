// Implements a structure that will map the real addresses to a virtual,
// consecutive mapping. This aims to avoid the issus of ASLR and of
// malloc potentially issuing different addresses.
//
// We can also use this to abstract `ptread_create/join` as reads-write
// to virtual addresses.

use std::num::NonZeroUsize;

use lotto::collections::FxHashMap;
use lotto::engine::handler::Address;
use lotto::log::trace;
use serde_derive::{Deserialize, Serialize};

#[derive(PartialEq, Serialize, Deserialize)]
pub struct VirtualAddressMapper {
    /// Maps from the address of this run to a new one to deal with
    /// the fact that addresses can be randomized.
    #[serde(skip)]
    real_address_to_virtual: FxHashMap<Address, Address>,
    /// Maps from our virtual given address to the one on this run,
    /// which may change.
    #[serde(skip)]
    virtual_address_to_real: FxHashMap<Address, Address>,
    virtual_address_cnt: u64,
}

impl VirtualAddressMapper {
    pub fn new(offset: u64) -> Self {
        Self {
            real_address_to_virtual: FxHashMap::default(),
            virtual_address_to_real: FxHashMap::default(),
            virtual_address_cnt: offset,
        }
    }
    pub fn get_virtual_address_cnt(&self) -> u64 {
        self.virtual_address_cnt
    }

    /// Creates a mapping, if it doesnt exist between a set of addresses.
    /// This is only used when we are rebuilding!
    pub fn add_mapping_in_rebuild(
        &mut self,
        real_address: Address,
        virtual_address: Address,
    ) -> Address {
        let old = self
            .real_address_to_virtual
            .insert(real_address, virtual_address);
        assert!(old.is_none() || old == Some(virtual_address));
        let old = self
            .virtual_address_to_real
            .insert(virtual_address, real_address);
        assert!(old.is_none() || old == Some(real_address));
        assert!(
            usize::from(NonZeroUsize::from(virtual_address)) <= (self.virtual_address_cnt as usize)
        );
        virtual_address
    }

    /// Tries to get the virtual address mapped if it exists.
    /// Else, it creates a mapping from the `real_address` to a new `id`, which is
    /// one more than the previous virtual address.
    ///
    /// Returns the virtual address
    pub fn get_virtual_address_or_create(&mut self, real_address: &Address) -> Address {
        if let Some(v_addr) = self.real_address_to_virtual.get(real_address) {
            return *v_addr;
        }
        let v_addr = self.get_next_virtual_address();
        let old = self.real_address_to_virtual.insert(*real_address, v_addr);
        assert!(old.is_none());
        let old = self.virtual_address_to_real.insert(v_addr, *real_address);
        assert!(old.is_none());
        v_addr
    }

    pub fn get_virtual_address(&self, real_address: &Address) -> Option<Address> {
        self.real_address_to_virtual.get(real_address).copied()
    }

    pub fn get_real_address(&self, virtual_addr: &Address) -> Address {
        self.virtual_address_to_real[virtual_addr]
    }

    fn get_next_virtual_address(&mut self) -> Address {
        if self.virtual_address_cnt as usize == usize::MAX {
            panic!("Exceeded the maximum number of virtual addresses!");
        }
        self.virtual_address_cnt += 1;
        Address::new(NonZeroUsize::new(self.virtual_address_cnt as usize).unwrap())
    }

    pub fn print_mapping(&self) {
        trace!("Printing mapping of real addresses to virtual for `VirtualAddressMapper`");
        for (r_addr, v_addr) in &self.real_address_to_virtual {
            trace!("Mapping {:?} into {:?}", r_addr, v_addr);
        }
    }
    pub fn is_empty(&self) -> bool {
        self.real_address_to_virtual.is_empty()
    }
}
