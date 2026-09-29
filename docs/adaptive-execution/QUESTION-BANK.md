# AIR Adaptive Execution - Question Bank

Status: OPEN DESIGN / TEST QUESTIONS

This is not a checklist to answer from theory. Questions should be retired by
source inspection, experiments, implementation evidence, or explicit deferral.

Priority legend:

- P0: answer before ownership/refactor
- P1: answer before the relevant wave closes
- P2: later pressure test / roadmap

## A. Current AIR and refactoring

1. [P0] Which AIR 0.10.0 types currently own canonical truth versus derived
   execution state?
2. [P0] Where are Qwen2 assumptions still present outside the architecture
   adapter?
3. [P0] Where are token/sequence assumptions embedded in serving?
4. [P0] Which scheduler decisions are truly generic admission decisions?
5. [P0] Which scheduler decisions exist only because autoregressive decode has
   prefill/decode phases?
6. [P0] Which KV-cache abstractions are reusable state concepts and which are
   transformer-specific?
7. [P0] Which current planner fields can survive a non-token workload?
8. [P0] Does any current metric become a second state authority?
9. [P0] Is `HardwareTopology` consumed anywhere important today, or mostly a
   qualified seed?
10. [P0] Which current APIs must remain stable during refactor?
11. [P0] What characterization tests are missing before splitting current types?
12. [P1] Which old paths can be deleted immediately after each ownership move?
13. [P1] Are there current duplicate model/tensor geometry calculations?
14. [P1] Can Reference remain independent if semantic operations become shared?
15. [P1] Which current internal types accidentally became public through headers?
16. [P1] What is the smallest migration path that avoids parallel generic and
    transformer runtimes?

## B. Hardware as data

17. [P0] What is stable topology versus dynamic environment state?
18. [P0] Should CPU cores be individual nodes, groups, or hierarchical resources?
19. [P0] How do we represent NUMA without overfitting to servers?
20. [P0] How do we distinguish addressable capacity from currently available
    capacity?
21. [P0] Where should cache hierarchy live?
22. [P0] How are PCIe/root-complex relationships observed reliably on Linux?
23. [P0] How do we represent integrated/shared-memory GPUs?
24. [P0] How do we represent devices with unified virtual memory?
25. [P0] What hardware facts can AIR trust from APIs versus infer from benchmarks?
26. [P1] How are copy engines represented?
27. [P1] How are concurrent copy/compute capabilities represented?
28. [P1] How are tensor/matrix acceleration capabilities represented?
29. [P1] How do we model CPU SIMD/AMX-like capabilities without a string bag?
30. [P1] How do we represent storage when weight streaming becomes relevant?
31. [P1] How do we represent peer GPU links and peer-access restrictions?
32. [P1] How do we fingerprint topology across driver/kernel changes?
33. [P2] How do remote accelerators appear without implying AIR supports
    distributed execution?
34. [P2] How do virtual machines/containers expose incomplete topology?
35. [P2] How do MIG/partitioned devices affect identity and capacity?

## C. Home versus enterprise systems

36. [P0] What assumptions are safe on one laptop but invalid on a workstation?
37. [P0] Does the same planner work on CPU-only systems?
38. [P0] Does the same planner work when CPU and GPU share memory?
39. [P1] How does plan selection change on 2-4 local GPUs?
40. [P1] How does NUMA placement affect host preparation and transfers?
41. [P1] What happens when enterprise GPUs have peer connectivity only in some
    pairs?
42. [P1] How do we avoid topology visualizations becoming unreadable on many
    devices?
43. [P1] How do we represent shared enterprise accelerators with competing
    tenants?
44. [P1] How do resource reservations interact with external schedulers?
45. [P2] What boundaries must be crossed before AIR becomes multi-host?
46. [P2] Is a remote accelerator an AIR execution resource or another MEF
    provider?
47. [P2] How would rack/cluster topology differ from local machine topology?
48. [P2] What data must never be assumed visible in managed/cloud environments?

## D. Timing, clocks, and causality

49. [P0] Which host clocks are used today?
50. [P0] How do we correlate CPU and GPU timestamps?
51. [P0] How do we measure kernel execution without forcing synchronization that
    changes performance?
52. [P0] How do we measure transfer overlap?
53. [P0] How do we distinguish queue delay, launch delay, execution time, and
    synchronization time?
54. [P0] How do we represent uncertainty in cross-clock correlation?
55. [P1] How much tracing overhead is acceptable in normal mode?
56. [P1] What profiling mode is permitted during qualification?
57. [P1] How do frequency scaling and boost clocks affect comparisons?
58. [P1] How long must a workload run before thermal steady state matters?
59. [P1] How do we detect plan evidence made stale by power/thermal change?
60. [P1] How do asynchronous errors map back to the operation that caused them?
61. [P2] Can hardware counters be sampled without privileged access?
62. [P2] How do we handle platforms where high-quality timing APIs differ?

## E. Memory and data movement

63. [P0] What buffers exist today and who owns each lifetime?
64. [P0] What allocations happen per request versus per model versus per plan?
65. [P0] When is pinned host memory beneficial on the qualified machine?
66. [P0] Where do hidden synchronization points currently prevent overlap?
67. [P1] Can weight transfers overlap useful computation?
68. [P1] When does double buffering improve throughput versus increase pressure?
69. [P1] How should AIR represent reusable memory arenas?
70. [P1] How is fragmentation measured?
71. [P1] How does AIR reason about offload when a multi-component model exceeds
    VRAM?
72. [P1] Can component residency be planned separately from operation placement?
73. [P1] When is recomputation cheaper than retaining state?
74. [P1] How are tensor/latent lifetimes proven safe for buffer reuse?
75. [P2] Can storage-to-device paths bypass host copies on supported systems?
76. [P2] How would compressed weight streaming affect the execution graph?

## F. Semantic computation

77. [P0] What is the smallest semantic representation required by Qwen2?
78. [P0] What does the second image workflow require that Qwen2 does not?
79. [P0] Which values need semantic types rather than only tensor shapes?
80. [P0] Are loops first-class semantic regions or workflow-level composition?
81. [P0] How are random-number semantics represented reproducibly?
82. [P0] Where do tokenization, image preprocessing, and schedulers live?
83. [P1] What is an operation versus a component?
84. [P1] What is a pure operation versus stateful operation?
85. [P1] Are model weights operation inputs, resource bindings, or both?
86. [P1] How is optional/control-flow behavior represented without a universal
    programming language?
87. [P1] Which shape dimensions may be symbolic?
88. [P1] Who resolves symbolic shapes and when?
89. [P1] How are dtype/precision semantics separated from physical tactic?
90. [P1] How is numerical tolerance part of a semantic contract, if at all?
91. [P2] What would recurrent/state-space models force us to add?
92. [P2] What would mixture-of-experts routing force us to add?

## G. Unknown architectures and extension

93. [P0] How does AIR recognize a package it can parse but cannot execute?
94. [P0] What exact structured failure describes missing semantics?
95. [P0] Can a package declare semantic operations without executing package code?
96. [P0] How are semantic operation identities versioned?
97. [P0] Who owns implementation registration?
98. [P0] How do we prevent extensions from becoming arbitrary plugin execution?
99. [P1] Can new operations be added without changing unrelated planner logic?
100. [P1] Must every new operation have a Reference implementation?
101. [P1] What if an operation is meaningful only on specialized hardware?
102. [P1] How do we distinguish unsupported semantics from unsupported hardware?
103. [P1] How are custom fused ops related to canonical semantic ops?
104. [P1] Can an extension supply a lowering rather than a new primitive?
105. [P2] What is the toolsmith workflow for a genuinely new semantic primitive?
106. [P2] Can extensions be sandboxed or capability-limited?

## H. Execution graph and scheduling

107. [P0] What is the minimum physical node set needed to represent current Qwen2
     execution?
108. [P0] Which dependencies must be explicit versus implied?
109. [P0] Who owns physical placement decisions?
110. [P0] How does the execution graph coexist with the current single scheduler?
111. [P0] What does "replay the same plan" mean when dynamic addresses differ?
112. [P1] How are CUDA streams/queues represented?
113. [P1] How are events/synchronization represented?
114. [P1] How are CPU tasks represented?
115. [P1] When can independent operations execute concurrently?
116. [P1] When is fusion semantically safe?
117. [P1] How is batch formation represented?
118. [P1] Can CUDA Graph capture be a physical implementation of a stable subgraph?
119. [P1] How does cancellation propagate through captured/replayed work?
120. [P1] How do plan transitions avoid double-reserving memory?
121. [P2] How do heterogeneous devices share one workload without hidden
     cross-device fallback?
122. [P2] How are device failures represented mid-plan?

## I. Objectives and autotuning

123. [P0] Which objective is the default and why?
124. [P0] How does a user express hard constraints versus preferences?
125. [P0] Can throughput improve while latency becomes unacceptable?
126. [P0] How are incomparable multi-objective candidates presented?
127. [P1] What candidate search space is safe to explore automatically?
128. [P1] How are warmup and cache effects controlled?
129. [P1] How many repetitions are needed before promotion?
130. [P1] How is measurement noise modeled?
131. [P1] How are thermal trends controlled?
132. [P1] What is the rollback condition for a promoted plan?
133. [P1] Can plan evidence be reused across nearby shapes?
134. [P1] When must evidence be invalidated by driver/toolchain changes?
135. [P2] Can AIR learn a cost model while retaining raw observations?
136. [P2] How do we prevent overfitting to one benchmark/workload?

## J. Data science and evidence quality

137. [P0] What metadata is required for every performance measurement?
138. [P0] Which values are distributions rather than scalars?
139. [P0] How are outliers retained rather than silently discarded?
140. [P0] How is measurement methodology versioned?
141. [P1] Which statistical summaries are appropriate for latency distributions?
142. [P1] How is confidence communicated without pretending certainty?
143. [P1] How are censored/failed/cancelled runs represented?
144. [P1] How do competing processes contaminate measurements?
145. [P1] How do we compare plans across different thermal/power regimes?
146. [P1] When is a benchmark representative of a production workload?
147. [P2] Can we build predictive cost models without making them authorities?
148. [P2] How do models of performance expose uncertainty to the planner?

## K. Image, video, audio, Laya, and Jev

149. [P0] Which concrete image workflow is the best second implementation on the
     qualified development hardware?
150. [P0] Can that workflow run externally first to establish an oracle?
151. [P0] Which components are independently loadable?
152. [P0] What loop/state semantics does denoising require?
153. [P1] How are scheduler/solver steps represented?
154. [P1] How are latent/image values represented?
155. [P1] How does component CPU/GPU offload affect planning?
156. [P1] Which image semantics deserve AIR primitives versus lowerings?
157. [P2] What genuinely new semantics does video add beyond larger tensors?
158. [P2] How are temporal chunking and frame decode lifetimes represented?
159. [P2] How should audio codecs/spectrogram/waveform semantics be represented?
160. [P2] How would synchronized audio/video outputs affect deadlines?
161. [P2] What is the canonical computational definition of Laya before AIR
     attempts support?
162. [P2] What is the canonical computational definition of Jev before AIR
     attempts support?
163. [P2] Which Laya/Jev requirements are new semantics versus compositions of
     existing operations?

## L. GUI and human control

164. [P0] What does a first-time user need to see in five seconds?
165. [P0] How do we show semantic architecture separately from physical execution?
166. [P0] How do we show facts separately from measurements/inferences?
167. [P0] How can a user answer "why is this slow?"
168. [P0] How do we visualize dependency-caused idle time?
169. [P1] How do we display a laptop topology without overwhelming the user?
170. [P1] How do we collapse an enterprise topology intelligently?
171. [P1] How do we display model component residency/offload?
172. [P1] How do we compare active versus candidate plans?
173. [P1] How do we show insufficient evidence rather than inventing a bottleneck?
174. [P1] What controls are safe for novice users?
175. [P1] What advanced controls expose objectives/constraints without exposing
     internal accidents?
176. [P1] How does the GUI reconnect and reconstruct current state?
177. [P1] How is GUI observer overhead shown?
178. [P1] What should the mobile surface omit?
179. [P2] Can the user export a complete execution/evidence bundle from the GUI?

## M. Reliability, security, and failure behavior

180. [P0] What happens when a device disappears after planning?
181. [P0] What happens when allocation succeeds during planning but fails during
     execution?
182. [P0] What happens when a driver resets?
183. [P0] What happens when measurement APIs return partial/invalid data?
184. [P0] What happens when a plan references stale topology?
185. [P0] Can malformed package metadata cause unbounded allocation?
186. [P0] How are untrusted package configs bounded?
187. [P0] How do we prevent arbitrary package-supplied code execution?
188. [P1] What execution state can be safely replayed after failure?
189. [P1] How do we retain partial failure evidence?
190. [P1] How does cancellation interact with asynchronous transfers?
191. [P1] How do plan transitions remain atomic enough for resource accounting?
192. [P1] How are OOM and overload distinguished?
193. [P1] How does AIR fail on unsupported precision/hardware combinations?
194. [P2] What isolation is needed for third-party compiled implementations?

## N. Builder, MEF, standalone use, and system boundaries

195. [P0] Can every new AIR feature still work without Builder?
196. [P0] Is any proposed AIR type actually system structure that belongs in
     Builder?
197. [P0] Is any proposed provider-selection logic actually MEF authority?
198. [P0] Does MEF need a richer capability than `text.generate@1.0.0` for image
     work, or is that a separate MEF program?
199. [P1] What would a future Builder-to-AIR compiler consume and emit?
200. [P1] Which AIR semantic identities should Builder reference without owning?
201. [P1] How does AIR expose qualification evidence back to MEF?
202. [P1] Can AIR remain a normal executable/library when no ecosystem components
     are installed?
203. [P2] How would an AIR execution artifact be content-addressed by Builder?
204. [P2] Which cross-project versioning rules are needed?

## O. AI coding-agent / project-management limits

205. [P0] Can a fresh agent understand the active wave from four small documents?
206. [P0] Are key invariants duplicated inconsistently across docs?
207. [P0] Is any current wave too large for one agent/context window?
208. [P0] Which source files are becoming context-hostile monoliths?
209. [P0] Where should large files be split by responsibility?
210. [P0] Do test names reveal the authority/contract they protect?
211. [P0] Does every task packet have an explicit stop condition?
212. [P1] Can subagents investigate independently without concurrent authority
     edits?
213. [P1] Are machine logs stored as artifacts instead of pasted into current docs?
214. [P1] Can a new agent retrieve only the evidence relevant to one failed gate?
215. [P1] Do commits remain small enough to audit and revert?
216. [P1] Is `CURRENT.md` describing state rather than becoming an architecture
     essay?
217. [P1] Is the question bank being retired with evidence rather than growing
     forever?
218. [P2] What repository tooling can automatically detect stale program docs?

## P. Release/qualification

219. [P0] What exact claims would AIR 0.11.0 be allowed to make?
220. [P0] Which 0.10.0 public contracts must remain compatible?
221. [P1] What new schemas require explicit versioning?
222. [P1] What frozen 0.10.0 baseline must be rerun as non-regression?
223. [P1] How do we qualify CPU-only and CUDA paths independently?
224. [P1] How do we destructively test topology/environment separation?
225. [P1] How do we prove the execution graph does not become semantic truth?
226. [P1] How do we prove GUI disconnect/reconnect does not alter execution?
227. [P1] How do we prove candidate-plan testing cannot corrupt active execution?
228. [P1] How do we prove stale evidence cannot promote a plan?
229. [P1] How do we qualify the second architecture without broad universal claims?
230. [P1] Does frozen MEF R0 `provider.air.http` remain compatible?
231. [P2] What should the public support matrix say about unknown architectures?
232. [P2] Which negative results are important enough to publish?
