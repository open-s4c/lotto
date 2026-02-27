/*
 */

#include <lotto/qlotto/qemu/armcpu.h>
#include <lotto/sys/assert.h>

/*
reg_num reg_name reg_bit_size
0	x0      64
1	x1      64
2	x2      64
3	x3      64
4	x4      64
5	x5      64
6	x6      64
7	x7      64
8	x8      64
9	x9      64
10	x10     64
11	x11     64
12	x12     64
13	x13     64
14	x14     64
15	x15     64
16	x16     64
17	x17     64
18	x18     64
19	x19     64
20	x20     64
21	x21     64
22	x22     64
23	x23     64
24	x24     64
25	x25     64
26	x26     64
27	x27     64
28	x28     64
29	x29     64
30	x30     64
31	sp      64
32	pc      64
33	cpsr        32
34	v0      128
35	v1      128
36	v2      128
37	v3      128
38	v4      128
39	v5      128
40	v6      128
41	v7      128
42	v8      128
43	v9      128
44	v10     128
45	v11     128
46	v12     128
47	v13     128
48	v14     128
49	v15     128
50	v16     128
51	v17     128
52	v18     128
53	v19     128
54	v20     128
55	v21     128
56	v22     128
57	v23     128
58	v24     128
59	v25     128
60	v26     128
61	v27     128
62	v28     128
63	v29     128
64	v30     128
65	v31     128
66	fpsr        32
67	fpcr        32
68	ID_MMFR5        64
69	RES_0_C0_C3_7       64
70	MAIR_EL3        64
71	ID_AA64PFR1_EL1     64
72	ESR_EL2     64
73	ID_AA64PFR2_EL1_RESERVED        64
74	VMPIDR_EL2      64
75	ID_AA64PFR3_EL1_RESERVED        64
76	REVIDR_EL1      64
77	ID_AA64ZFR0_EL1     64
78	DACR32_EL2      64
79	ID_AA64SMFR0_EL1        64
80	CPACR       64
81	CNTKCTL     64
82	ID_AA64PFR6_EL1_RESERVED        64
83	FPEXC32_EL2     64
84	ID_AA64PFR7_EL1_RESERVED        64
85	ID_AA64DFR0_EL1     64
86	ID_AA64DFR1_EL1     64
87	ID_AA64DFR2_EL1_RESERVED        64
88	ID_AA64DFR3_EL1_RESERVED        64
89	ACTLR_EL1       64
90	ID_AA64AFR0_EL1     64
91	ID_AA64AFR1_EL1     64
92	ID_AA64AFR2_EL1_RESERVED        64
93	CNTFRQ_EL0      64
94	OSDTRRX_EL1     64
95	ID_AA64AFR3_EL1_RESERVED        64
96	SCTLR       64
97	SPSR_EL1        64
98	ID_AA64ISAR0_EL1        64
99	DBGBVR0_EL1     64
100	ID_AA64ISAR1_EL1        64
101	ELR_EL1     64
102	FAR_EL2     64
103	PMEVTYPER0_EL0      64
104	DBGBCR0_EL1     64
105	ID_AA64ISAR2_EL1_RESERVED       64
106	PMEVTYPER1_EL0      64
107	DBGWVR0_EL1     64
108	ID_AA64ISAR3_EL1_RESERVED       64
109	VBAR        64
110	DBGWCR0_EL1     64
111	PMEVTYPER2_EL0      64
112	ID_AA64ISAR4_EL1_RESERVED       64
113	PMEVTYPER3_EL0      64
114	ID_AA64ISAR5_EL1_RESERVED       64
115	PMEVTYPER4_EL0      64
116	ID_AA64ISAR6_EL1_RESERVED       64
117	PMEVTYPER5_EL0      64
118	ID_AA64ISAR7_EL1_RESERVED       64
119	CNTVOFF_EL2     64
120	ID_AA64MMFR0_EL1        64
121	SP_EL0      64
122	DBGBVR1_EL1     64
123	ID_AA64MMFR1_EL1        64
124	DBGBCR1_EL1     64
125	ID_AA64MMFR2_EL1        64
126	PMINTENSET_EL1      64
127	DBGWVR1_EL1     64
128	ID_AA64MMFR3_EL1_RESERVED       64
129	DBGWCR1_EL1     64
130	ID_AA64MMFR4_EL1_RESERVED       64
131	PMCNTENSET_EL0      64
132	PMCR_EL0        64
133	ID_AA64MMFR5_EL1_RESERVED       64
134	PMCNTENCLR_EL0      64
135	SCTLR_EL2       64
136	ID_AA64MMFR6_EL1_RESERVED       64
137	PMOVSCLR_EL0        64
138	MDSCR_EL1       64
139	ID_AA64MMFR7_EL1_RESERVED       64
140	CNTHCTL_EL2     64
141	PMSELR_EL0      64
142	DBGBVR2_EL1     64
143	DBGBCR2_EL1     64
144	PMCEID0_EL0     64
145	PMCEID1_EL0     64
146	DBGWVR2_EL1     64
147	PMCCNTR_EL0     64
148	HCR_EL2     64
149	DBGWCR2_EL1     64
150	MDCR_EL2        64
151	CPTR_EL2        64
152	L2ACTLR     64
153	OSDTRTX_EL1     64
154	TTBR0_EL1       64
155	MDCCSR_EL0      64
156	TCR_EL1     64
157	ELR_EL2     64
158	SPSR_EL2        64
159	CNTHP_CTL_EL2       64
160	DBGBCR3_EL1     64
161	DBGBVR3_EL1     64
162	HACR_EL2        64
163	HPFAR_EL2       64
164	CNTHP_CVAL_EL2      64
165	DBGWVR3_EL1     64
166	PMUSERENR_EL0       64
167	DBGWCR3_EL1     64
168	RVBAR_EL2       64
169	CNTV_CVAL_EL0       64
170	TTBR1_EL1       64
171	VBAR_EL2        64
172	PMOVSSET_EL0        64
173	CNTV_CTL_EL0        64
174	ACTLR_EL2       64
175	MDRAR_EL1       64
176	SP_EL1      64
177	PMCCFILTR_EL0       64
178	DBGBVR4_EL1     64
179	DBGBCR4_EL1     64
180	DBGCLAIMSET_EL1     64
181	HSTR_EL2        64
182	CPUACTLR_EL1        64
183	CNTP_CTL_EL0        64
184	CPUECTLR_EL1        64
185	CONTEXTIDR_EL1      64
186	CNTP_CVAL_EL0       64
187	CPUMERRSR_EL1       64
188	CNTPS_CVAL_EL1      64
189	L2MERRSR_EL1        64
190	DBGBVR5_EL1     64
191	MAIR_EL1        64
192	ACTLR_EL3       64
193	DBGBCR5_EL1     64
194	CNTPS_CTL_EL1       64
195	TPIDR_EL1       64
196	DBGCLAIMCLR_EL1     64
197	AFSR0_EL1       64
198	SP_EL2      64
199	OSLSR_EL1       64
200	AFSR1_EL1       64
201	PAR_EL1     64
202	CBAR_EL1        64
203	OSECCR_EL1      64
204	SPSR_IRQ        64
205	MDCR_EL3        64
206	SPSR_ABT        64
207	FPCR        64
208	SPSR_UND        64
209	FPSR        64
210	SPSR_FIQ        64
211	CLIDR       64
212	ESR_EL1     64
213	ID_PFR0     64
214	TCR_EL2     64
215	ID_DFR0     64
216	AMAIR0      64
217	VTTBR_EL2       64
218	ID_AFR0     64
219	VTCR_EL2        64
220	ID_MMFR0        64
221	CSSELR      64
222	TTBR0_EL2       64
223	ID_MMFR1        64
224	AIDR        64
225	TPIDR_EL0       64
226	ID_MMFR2        64
227	TPIDRRO_EL0     64
228	ID_MMFR3        64
229	OSDLR_EL1       64
230	IFSR32_EL2      64
231	ID_ISAR0        64
232	ID_ISAR1        64
233	PMEVCNTR0_EL0       64
234	ID_ISAR2        64
235	PMEVCNTR1_EL0       64
236	ID_ISAR3        64
237	CTR_EL0     64
238	TPIDR_EL2       64
239	PMEVCNTR2_EL0       64
240	ID_ISAR4        64
241	PMEVCNTR3_EL0       64
242	ID_ISAR5        64
243	MAIR_EL2        64
244	PMEVCNTR4_EL0       64
245	ID_MMFR4        64
246	AFSR0_EL2       64
247	PMEVCNTR5_EL0       64
248	ID_ISAR6        64
249	AFSR1_EL2       64
250	L2CTLR_EL1      64
251	MVFR0_EL1       64
252	VPIDR_EL2       64
253	L2ECTLR_EL1     64
254	MVFR1_EL1       64
255	FAR_EL1     64
256	RES_0_C0_C3_3       64
257	MVFR2_EL1       64
258	ID_PFR2     64
259	AMAIR_EL2       64
260	ID_DFR1     64
*/

static inline uint32_t
vfp_get_fpsr(CPUARMState *env)
{
    return vfp_get_fpscr(env) & FPSR_MASK;
}

static inline uint32_t
vfp_get_fpcr(CPUARMState *env)
{
    return vfp_get_fpscr(env) & FPCR_MASK;
}

uint32_t
cpsr_read(CPUARMState *env)
{
    int ZF;
    ZF = (env->ZF == 0);
    return env->uncached_cpsr | (env->NF & 0x80000000) | (ZF << 30) |
           (env->CF << 29) | ((env->VF & 0x80000000) >> 3) | (env->QF << 27) |
           (env->thumb << 5) | ((env->condexec_bits & 3) << 25) |
           ((env->condexec_bits & 0xfc) << 8) | (env->GE << 16) |
           (env->daif & CPSR_AIF);
}

/* Convert host exception flags to vfp form.  */
static inline uint32_t
vfp_exceptbits_from_host(uint32_t host_bits)
{
    uint32_t target_bits = 0;

    if (host_bits & float_flag_invalid) {
        target_bits |= 1;
    }
    if (host_bits & float_flag_divbyzero) {
        target_bits |= 2;
    }
    if (host_bits & float_flag_overflow) {
        target_bits |= 4;
    }
    if (host_bits & (float_flag_underflow | float_flag_output_denormal)) {
        target_bits |= 8;
    }
    if (host_bits & float_flag_inexact) {
        target_bits |= 0x10;
    }
    if (host_bits & float_flag_input_denormal) {
        target_bits |= 0x80;
    }
    return target_bits;
}

static inline int
get_float_exception_flags(float_status *status)
{
    return status->float_exception_flags;
}

static uint32_t
vfp_get_fpscr_from_host(CPUARMState *env)
{
    uint32_t i;

    i = get_float_exception_flags(&env->vfp.fp_status);
    i |= get_float_exception_flags(&env->vfp.standard_fp_status);
    /* FZ16 does not generate an input denormal exception.  */
    i |= (get_float_exception_flags(&env->vfp.fp_status_f16) &
          ~float_flag_input_denormal);
    i |= (get_float_exception_flags(&env->vfp.standard_fp_status_f16) &
          ~float_flag_input_denormal);
    return vfp_exceptbits_from_host(i);
}

uint32_t
vfp_get_fpscr(CPUARMState *env)
{
    uint32_t i, fpscr;

    fpscr = env->vfp.xregs[ARM_VFP_FPSCR] | (env->vfp.vec_len << 16) |
            (env->vfp.vec_stride << 20);

    /*
     * M-profile LTPSIZE overlaps A-profile Stride; whichever of the
     * two is not applicable to this CPU will always be zero.
     */
    fpscr |= env->v7m.ltpsize << 16;

    fpscr |= vfp_get_fpscr_from_host(env);

    i = env->vfp.qc[0] | env->vfp.qc[1] | env->vfp.qc[2] | env->vfp.qc[3];
    fpscr |= i ? FPCR_QC : 0;

    return fpscr;
}

/* return size in bits */
uint64_t
gdb_get_register_size(uint64_t reg_num)
{
    ASSERT(reg_num <= 260);

    // x0 - x30, sp, pc
    if (reg_num <= 32)
        return 64;

    // cpsr
    if (reg_num == 33)
        return 32;

    // v0-v31
    if (reg_num >= 34 && reg_num <= 65)
        return 128;

    if (reg_num >= 66 && reg_num <= 260)
        return 64;

    ASSERT(0 && "reg_num unknown");
    return 0;
}

uint128_t
gdb_get_register_value(CPUARMState *cpu, uint64_t reg_num)
{
    ASSERT(reg_num <= 260);

    uint128_t reg_value = 0;
    uint64_t reg_size   = gdb_get_register_size(reg_num);

    uint128_t value_mask = 0;
    switch (reg_size) {
        case 32:
            value_mask = 0xFFFFFFFF;
            break;
        case 64:
            value_mask = 0xFFFFFFFFFFFFFFFF;
            break;
        case 128:
            value_mask = 0;
            break;
        default:
            ASSERT(0 && "invalid register size");
            break;
    }

    /* Getting Value */

    // read 'x' register values
    if (reg_num <= 30) {
        reg_value = cpu->xregs[reg_num];
        if (value_mask > 0)
            reg_value &= value_mask;

        return reg_value;
    }

    if (reg_num >= 68 && reg_num <= 260)
        reg_value = 0;

    // read 'v' fpu register values
    if (reg_num >= 34 && reg_num <= 65) {
        reg_value = 0;
        if (value_mask > 0)
            reg_value &= value_mask;
        return reg_value;
    }

    switch (reg_num) {
        case 31: {
            uint64_t sp = 0;
            if (((cpu->pstate >> 2) & 0x3) == 0)
                sp = cpu->sp_el[0];
            else
                sp = cpu->sp_el[1];
            reg_value = sp;
            break;
        }
        // pc
        case 32:
            reg_value = cpu->pc;
            break;
        // cpsr
        case 33:
            reg_value = cpsr_read(cpu);
            break;

        // fpsr
        case 66:
            reg_value = vfp_get_fpsr(cpu);
            break;
        // fpcr
        case 67:
            reg_value = vfp_get_fpcr(cpu);
            break;
        // ESR_EL2
        case 72:
            reg_value = cpu->cp15.esr_el[2];
            break;
        // ELR_EL1
        case 101:
            reg_value = cpu->elr_el[1];
            break;
        // FAR_EL2
        case 102:
            reg_value = cpu->cp15.far_el[2];
            break;
        // SP_EL0
        case 121:
            reg_value = cpu->sp_el[0];
            break;
        // CPTR_EL2
        case 151:
            reg_value = cpu->cp15.cptr_el[2];
            break;
        // TTBR0_EL1
        case 154:
            reg_value = cpu->cp15.ttbr0_el[1];
            break;
        // TCR_EL1
        case 156:
            reg_value = cpu->cp15.tcr_el[1];
            break;
        // ELR_EL2
        case 157:
            reg_value = cpu->elr_el[2];
            break;
        // TTBR1_EL1
        case 170:
            reg_value = cpu->cp15.ttbr1_el[1];
            break;
        // SP_EL1
        case 176:
            reg_value = cpu->sp_el[1];
            break;
        // CONTEXTIDR_EL1
        case 185:
            reg_value = cpu->cp15.contextidr_el[1];
            break;
        // TPIDR_EL1
        case 195:
            reg_value = cpu->cp15.tpidr_el[1];
            break;
        // SP_EL1
        case 198:
            reg_value = cpu->sp_el[2];
            break;
        // TCR_EL1
        case 214:
            reg_value = cpu->cp15.tcr_el[2];
            break;
        // TTBR0_EL2
        case 222:
            reg_value = cpu->cp15.ttbr0_el[2];
            break;
        // TPIDR_EL0
        case 225:
            reg_value = cpu->cp15.tpidr_el[0];
            break;
        // TPIDRRO_EL0
        case 227:
            reg_value = cpu->cp15.tpidrro_el[0];
            break;
        // TPIDR_EL2
        case 238:
            reg_value = cpu->cp15.tpidr_el[2];
            break;
        // FAR_EL1
        case 255:
            reg_value = cpu->cp15.far_el[1];
            break;
        default:
            reg_value = 0;
            break;
    }

    /* Got Value */

    if (value_mask > 0)
        reg_value &= value_mask;

    return reg_value;
}
