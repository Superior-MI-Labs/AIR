#include "air/cuda.hpp"
#include "air/reference.hpp"
#include "air/serving.hpp"
#include "air/storage.hpp"
#include "cuda/cuda_executor_factory.hpp"
#include "model/prepared_model.hpp"
#include "runtime/backend.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <initializer_list>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

class ModelBuilder {
public:
    void add_f32(std::string name,
                 std::vector<std::uint64_t> shape,
                 std::span<const float> values) {
        std::vector<std::byte> raw;
        raw.reserve(values.size() * 4U);
        for (const float value : values) {
            const auto bits = std::bit_cast<std::uint32_t>(value);
            for (unsigned shift = 0; shift < 32U; shift += 8U) {
                raw.push_back(static_cast<std::byte>((bits >> shift) & 0xffU));
            }
        }

        air::TensorDescriptor descriptor;
        descriptor.name = std::move(name);
        descriptor.type = air::DataType::f32;
        descriptor.format_type = 0;
        descriptor.shape.dimensions = std::move(shape);
        descriptor.byte_offset = bytes_.size();
        descriptor.byte_size = raw.size();
        descriptor.byte_size_exact = true;
        tensors_.push_back(std::move(descriptor));
        bytes_.insert(bytes_.end(), raw.begin(), raw.end());
    }

    std::shared_ptr<const air::ModelDefinition> finish(
        air::ModelConfig config,
        air::TokenizerDefinition tokenizer) {
        const auto path =
            std::filesystem::temp_directory_path() / "air-cuda-contract-fixture.bin";
        {
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            out.write(reinterpret_cast<const char*>(bytes_.data()),
                      static_cast<std::streamsize>(bytes_.size()));
        }

        auto storage = air::ModelStorage::map_read_only(path);
        if (!storage) {
            throw std::runtime_error(storage.status().message());
        }
        std::filesystem::remove(path);

        return std::make_shared<air::ModelDefinition>(
            air::ModelFingerprint{"test", config.architecture, "cuda-contract-fixture"},
            std::move(config),
            std::move(tokenizer),
            std::move(tensors_),
            std::move(storage).value());
    }

private:
    std::vector<std::byte> bytes_;
    std::vector<air::TensorDescriptor> tensors_;
};

std::vector<float> zeros(std::size_t count) {
    return std::vector<float>(count, 0.0F);
}

std::vector<float> ones(std::size_t count) {
    return std::vector<float>(count, 1.0F);
}

std::shared_ptr<const air::ModelDefinition> tiny_qwen2() {
    constexpr std::uint32_t embedding = 4;
    constexpr std::uint32_t vocab = 4;
    constexpr std::uint32_t ffn = 6;

    air::ModelConfig config;
    config.architecture = "qwen2";
    config.layer_count = 1;
    config.embedding_size = embedding;
    config.feed_forward_size = ffn;
    config.attention_head_count = 2;
    config.kv_head_count = 1;
    config.rope_dimension_count = 2;
    config.context_length = 8;
    config.vocabulary_size = vocab;
    config.rope_frequency_base = 10000.0;
    config.rms_norm_epsilon = 1.0e-5;

    air::TokenizerDefinition tokenizer;
    tokenizer.model = "gpt2";
    tokenizer.pre_tokenizer = "gpt2";
    tokenizer.vocabulary = {"a", "b", "c", "d"};
    tokenizer.token_types = {1, 1, 1, 1};
    tokenizer.special_ids.eos = 3;

    const std::vector<float> identity = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    ModelBuilder builder;
    builder.add_f32("token_embd.weight", {embedding, vocab}, identity);
    builder.add_f32("output_norm.weight", {embedding}, ones(embedding));
    builder.add_f32("blk.0.attn_norm.weight", {embedding}, ones(embedding));
    builder.add_f32("blk.0.attn_q.weight", {embedding, embedding},
                    zeros(embedding * embedding));
    builder.add_f32("blk.0.attn_k.weight", {embedding, 2},
                    zeros(embedding * 2));
    builder.add_f32("blk.0.attn_v.weight", {embedding, 2},
                    zeros(embedding * 2));
    builder.add_f32("blk.0.attn_output.weight", {embedding, embedding},
                    zeros(embedding * embedding));
    builder.add_f32("blk.0.ffn_norm.weight", {embedding}, ones(embedding));
    builder.add_f32("blk.0.ffn_gate.weight", {embedding, ffn},
                    zeros(embedding * ffn));
    builder.add_f32("blk.0.ffn_up.weight", {embedding, ffn},
                    zeros(embedding * ffn));
    builder.add_f32("blk.0.ffn_down.weight", {ffn, embedding},
                    zeros(ffn * embedding));

    return builder.finish(std::move(config), std::move(tokenizer));
}

struct AliasSemanticFixture {
    std::shared_ptr<const air::ModelDefinition> model;
    air::detail::PreparedModelSemantics prepared;
};

AliasSemanticFixture alias_qwen2_semantics() {
    constexpr std::uint32_t embedding = 4;
    constexpr std::uint32_t vocab = 4;
    constexpr std::uint32_t ffn = 6;

    air::ModelConfig config;
    config.architecture = "qwen2";
    config.layer_count = 1;
    config.embedding_size = embedding;
    config.feed_forward_size = ffn;
    config.attention_head_count = 2;
    config.kv_head_count = 1;
    config.rope_dimension_count = 2;
    config.context_length = 8;
    config.vocabulary_size = vocab;
    config.rope_frequency_base = 10000.0;
    config.rms_norm_epsilon = 1.0e-5;

    air::TokenizerDefinition tokenizer;
    tokenizer.model = "gpt2";
    tokenizer.pre_tokenizer = "gpt2";
    tokenizer.vocabulary = {"a", "b", "c", "d"};
    tokenizer.token_types = {1, 1, 1, 1};
    tokenizer.special_ids.eos = 3;

    const std::vector<float> identity = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    ModelBuilder builder;
    builder.add_f32("semantic.embedding.source", {embedding, vocab}, identity);
    builder.add_f32("semantic.final.norm.source", {embedding}, ones(embedding));
    builder.add_f32("semantic.layer0.attn.norm", {embedding}, ones(embedding));
    builder.add_f32("semantic.layer0.query", {embedding, embedding},
                    zeros(embedding * embedding));
    builder.add_f32("semantic.layer0.key", {embedding, 2}, zeros(embedding * 2));
    builder.add_f32("semantic.layer0.value", {embedding, 2}, zeros(embedding * 2));
    builder.add_f32("semantic.layer0.attn.out", {embedding, embedding},
                    zeros(embedding * embedding));
    builder.add_f32("semantic.layer0.ffn.norm", {embedding}, ones(embedding));
    builder.add_f32("semantic.layer0.ffn.gate", {embedding, ffn},
                    zeros(embedding * ffn));
    builder.add_f32("semantic.layer0.ffn.up", {embedding, ffn},
                    zeros(embedding * ffn));
    builder.add_f32("semantic.layer0.ffn.down", {ffn, embedding},
                    zeros(ffn * embedding));

    auto model = builder.finish(config, tokenizer);

    air::detail::PreparedModelSemantics prepared;
    prepared.source_model = model.get();
    prepared.architecture = "qwen2";
    prepared.geometry = config;
    prepared.head_dimension = 2;
    prepared.token_embedding_weight =
        model->find_tensor("semantic.embedding.source");
    prepared.output_norm_weight =
        model->find_tensor("semantic.final.norm.source");
    prepared.output_weight = prepared.token_embedding_weight;
    prepared.output_weight_tied = true;

    air::detail::PreparedLayerTensorBindings layer;
    layer.attention_norm_weight =
        model->find_tensor("semantic.layer0.attn.norm");
    layer.query_weight = model->find_tensor("semantic.layer0.query");
    layer.key_weight = model->find_tensor("semantic.layer0.key");
    layer.value_weight = model->find_tensor("semantic.layer0.value");
    layer.attention_output_weight =
        model->find_tensor("semantic.layer0.attn.out");
    layer.ffn_norm_weight =
        model->find_tensor("semantic.layer0.ffn.norm");
    layer.ffn_gate_weight =
        model->find_tensor("semantic.layer0.ffn.gate");
    layer.ffn_up_weight =
        model->find_tensor("semantic.layer0.ffn.up");
    layer.ffn_down_weight =
        model->find_tensor("semantic.layer0.ffn.down");
    prepared.layers.push_back(layer);

    return {std::move(model), std::move(prepared)};
}

bool near(float left, float right, float tolerance = 1.0e-4F) {
    return std::fabs(left - right) <= tolerance;
}

bool same_logits(std::span<const float> actual,
                 std::span<const float> expected,
                 const char* label) {
    if (actual.size() != expected.size()) {
        std::cerr << label << " logit width mismatch\n";
        return false;
    }
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (!near(actual[i], expected[i])) {
            std::cerr << label << " logit mismatch at index " << i
                      << ": cuda=" << actual[i]
                      << " reference=" << expected[i] << '\n';
            return false;
        }
    }
    return true;
}

air::TokenId greedy_token(std::span<const float> logits) {
    return static_cast<air::TokenId>(
        std::distance(logits.begin(),
                      std::max_element(logits.begin(), logits.end())));
}

int verify_cuda_operation_capabilities() {
    using Site = air::QualifiedOperationSite;

    auto prepared = air::runtime_detail::prepare_cuda_model(tiny_qwen2(), 0);
    if (!prepared) {
        std::cerr << "CUDA prepared-model capability fixture failed: "
                  << prepared.status().message() << '\n';
        return 1;
    }

    const auto& capabilities = prepared.value()->capabilities();
    if (capabilities.backend != air::BackendKind::cuda) {
        std::cerr << "prepared CUDA backend reported wrong backend identity\n";
        return 1;
    }

    const auto exact_linear = [&](Site site,
                                  std::initializer_list<air::QuantizedLinearExecutionKind> expected,
                                  const char* label) -> bool {
        auto actual = air::linear_implementations(capabilities, site);
        if (!actual) {
            std::cerr << label << " legality query failed: "
                      << actual.status().message() << '\n';
            return false;
        }
        if (actual.value().size() != expected.size() ||
            !std::equal(actual.value().begin(), actual.value().end(), expected.begin())) {
            std::cerr << label << " advertised unexpected linear implementation set\n";
            return false;
        }
        return true;
    };

    const auto exact_attention = [&](Site site,
                                     std::initializer_list<air::AttentionExecutionKind> expected,
                                     const char* label) -> bool {
        auto actual = air::attention_implementations(capabilities, site);
        if (!actual) {
            std::cerr << label << " legality query failed: "
                      << actual.status().message() << '\n';
            return false;
        }
        if (actual.value().size() != expected.size() ||
            !std::equal(actual.value().begin(), actual.value().end(), expected.begin())) {
            std::cerr << label << " advertised unexpected attention implementation set\n";
            return false;
        }
        return true;
    };

    if (!exact_linear(
            Site::prefill_transformer_block_linear,
            {
                air::QuantizedLinearExecutionKind::baseline,
                air::QuantizedLinearExecutionKind::batch_reuse4,
                air::QuantizedLinearExecutionKind::batch_reuse8,
                air::QuantizedLinearExecutionKind::dense_f32_cublas,
            },
            "CUDA prefill block-linear")) {
        return 1;
    }
    if (!exact_linear(
            Site::decode_transformer_block_linear,
            {
                air::QuantizedLinearExecutionKind::baseline,
                air::QuantizedLinearExecutionKind::batch_reuse8,
                air::QuantizedLinearExecutionKind::dense_f32_cublas,
            },
            "CUDA decode block-linear")) {
        return 1;
    }
    if (!exact_linear(
            Site::decode_output_projection,
            {
                air::QuantizedLinearExecutionKind::baseline,
                air::QuantizedLinearExecutionKind::batch_reuse8,
            },
            "CUDA decode output projection")) {
        return 1;
    }
    if (!exact_attention(
            Site::prefill_attention,
            {
                air::AttentionExecutionKind::baseline,
                air::AttentionExecutionKind::online_softmax,
            },
            "CUDA prefill attention")) {
        return 1;
    }
    if (!exact_attention(
            Site::decode_attention,
            {
                air::AttentionExecutionKind::baseline,
            },
            "CUDA decode attention")) {
        return 1;
    }

    if (air::linear_implementations(capabilities, Site::prefill_attention) ||
        air::attention_implementations(capabilities, Site::decode_output_projection)) {
        std::cerr << "CUDA operation legality accepted an implementation-family mismatch\n";
        return 1;
    }

    std::cout << "CUDA prepared backend operation-site legality passed\n";
    return 0;
}

int verify_cuda_alias_semantics() {
    auto canonical_model = tiny_qwen2();
    auto canonical_reference = air::ReferenceExecutor::create(canonical_model);
    if (!canonical_reference) {
        std::cerr << "canonical reference fixture failed: "
                  << canonical_reference.status().message() << '\n';
        return 1;
    }

    auto alias = alias_qwen2_semantics();
    auto semantic_status =
        air::detail::validate_prepared_model_semantics(alias.prepared);
    if (!semantic_status) {
        std::cerr << "alias prepared semantics failed validation: "
                  << semantic_status.message() << '\n';
        return 1;
    }

    if (alias.model->find_tensor("token_embd.weight") ||
        alias.model->find_tensor("blk.0.attn_q.weight") ||
        alias.model->find_tensor("output_norm.weight")) {
        std::cerr << "alias CUDA fixture unexpectedly contains canonical Qwen2 names\n";
        return 1;
    }

    auto alias_cuda = air::detail::CudaExecutorFactory::create(
        alias.model, 0, alias.prepared);
    if (!alias_cuda) {
        std::cerr << "alias CUDA executor failed: "
                  << alias_cuda.status().message() << '\n';
        return 1;
    }

    auto tactic_status = alias_cuda.value()->prepare_linear_tactic(
        air::QuantizedLinearExecutionKind::dense_f32_cublas);
    if (!tactic_status) {
        std::cerr << "alias dense-FP32 semantic tactic preparation failed: "
                  << tactic_status.message() << '\n';
        return 1;
    }

    air::ReferenceKvCache reference_cache(1, 1, 2, 8);
    auto expected = canonical_reference.value()->step(2, reference_cache);
    auto cuda_cache = alias_cuda.value()->create_kv_cache();
    if (!expected || !cuda_cache) {
        std::cerr << "alias CUDA semantic fixture setup failed\n";
        return 1;
    }

    auto actual = alias_cuda.value()->step(
        2,
        *cuda_cache.value(),
        air::QuantizedLinearExecutionKind::dense_f32_cublas,
        air::QuantizedLinearExecutionKind::baseline,
        air::AttentionExecutionKind::baseline);
    if (!actual) {
        std::cerr << "alias CUDA semantic execution failed: "
                  << actual.status().message() << '\n';
        return 1;
    }
    if (!same_logits(actual.value(), expected.value(),
                     "renamed semantic CUDA decode")) {
        return 1;
    }

    std::cout << "CUDA execution and dense tactic are independent of Qwen2 source names\n";
    return 0;
}

int verify_cuda_reference_parity() {
    auto model = tiny_qwen2();

    auto reference = air::ReferenceExecutor::create(model);
    if (!reference) {
        std::cerr << "reference fixture failed: "
                  << reference.status().message() << '\n';
        return 1;
    }

    auto cuda = air::CudaExecutor::create(model, 0);
    if (!cuda) {
        std::cerr << "CUDA fixture failed: "
                  << cuda.status().message() << '\n';
        return 1;
    }

    {
        air::ReferenceKvCache reference_cache(1, 1, 2, 8);
        auto cuda_cache = cuda.value()->create_kv_cache();
        if (!cuda_cache) {
            std::cerr << "CUDA single-decode KV fixture failed: "
                      << cuda_cache.status().message() << '\n';
            return 1;
        }

        auto expected = reference.value()->step(2, reference_cache);
        auto actual = cuda.value()->step(2, *cuda_cache.value());
        if (!expected || !actual) {
            std::cerr << "single decode failed: reference="
                      << (expected ? "ok" : expected.status().message())
                      << " cuda="
                      << (actual ? "ok" : actual.status().message()) << '\n';
            return 1;
        }
        if (!same_logits(actual.value(), expected.value(), "single decode")) {
            return 1;
        }
    }

    {
        const std::vector<air::TokenId> prompt = {0, 2};
        air::ReferenceKvCache reference_cache(1, 1, 2, 8);
        auto cuda_cache = cuda.value()->create_kv_cache();
        if (!cuda_cache) {
            std::cerr << "CUDA prefill KV fixture failed: "
                      << cuda_cache.status().message() << '\n';
            return 1;
        }

        auto expected = reference.value()->prefill(prompt, reference_cache);
        auto actual = cuda.value()->prefill(prompt, *cuda_cache.value());
        if (!expected || !actual) {
            std::cerr << "single-sequence prefill failed\n";
            return 1;
        }
        if (!same_logits(actual.value(), expected.value(), "single prefill")) {
            return 1;
        }
    }

    {
        auto cuda_cache_a = cuda.value()->create_kv_cache();
        auto cuda_cache_b = cuda.value()->create_kv_cache();
        if (!cuda_cache_a || !cuda_cache_b) {
            std::cerr << "CUDA batched-decode KV fixture failed\n";
            return 1;
        }

        std::vector<air::TokenId> tokens = {1, 2};
        std::vector<air::CudaKvCache*> caches = {
            cuda_cache_a.value().get(),
            cuda_cache_b.value().get(),
        };
        auto actual = cuda.value()->step_greedy_batch(tokens, caches);
        if (!actual || actual.value().size() != tokens.size()) {
            std::cerr << "CUDA batched decode failed\n";
            return 1;
        }

        for (std::size_t i = 0; i < tokens.size(); ++i) {
            air::ReferenceKvCache reference_cache(1, 1, 2, 8);
            auto expected = reference.value()->step(tokens[i], reference_cache);
            if (!expected ||
                actual.value()[i] != greedy_token(expected.value())) {
                std::cerr << "CUDA/reference batched decode greedy mismatch at item "
                          << i << '\n';
                return 1;
            }
        }
    }

    {
        const std::vector<air::TokenId> prompt_a = {0, 1};
        const std::vector<air::TokenId> prompt_b = {2, 3};
        auto cuda_cache_a = cuda.value()->create_kv_cache();
        auto cuda_cache_b = cuda.value()->create_kv_cache();
        if (!cuda_cache_a || !cuda_cache_b) {
            std::cerr << "CUDA multi-prefill KV fixture failed\n";
            return 1;
        }

        std::vector<air::CudaPrefillBatchItem> items;
        items.push_back({
            cuda_cache_a.value().get(),
            std::span<const air::TokenId>(prompt_a),
            air::CudaPrefillBatchOutput::logits,
        });
        items.push_back({
            cuda_cache_b.value().get(),
            std::span<const air::TokenId>(prompt_b),
            air::CudaPrefillBatchOutput::logits,
        });

        auto actual = cuda.value()->prefill_batch(items);
        if (!actual || actual.value().items.size() != 2U) {
            std::cerr << "CUDA multi-sequence prefill failed\n";
            return 1;
        }

        for (std::size_t i = 0; i < 2U; ++i) {
            air::ReferenceKvCache reference_cache(1, 1, 2, 8);
            const auto& prompt = i == 0U ? prompt_a : prompt_b;
            auto expected = reference.value()->prefill(prompt, reference_cache);
            if (!expected ||
                !same_logits(actual.value().items[i].logits,
                             expected.value(),
                             "multi-sequence prefill")) {
                return 1;
            }
        }
    }

    std::cout << "CUDA/reference semantic-binding parity passed for decode and prefill paths\n";
    return 0;
}

int verify_cuda_execution_graph_observation() {
    auto model =
        std::const_pointer_cast<air::ModelDefinition>(tiny_qwen2());

    air::SchedulerConfig scheduler;
    scheduler.max_active_requests = 2U;
    scheduler.max_queued_requests = 8U;
    scheduler.token_budget_per_cycle = 8U;
    scheduler.prefill_quantum_tokens = 2U;
    scheduler.cuda_kv_page_tokens = 2U;
    scheduler.prefix_cache_entries = 0U;
    scheduler.execution_observation_level =
        air::ExecutionObservationLevel::detailed;
    scheduler.execution_span_capacity = 512U;

    // Native decode batching is intentionally not selected for a fully
    // baseline decode plan. Exercise the production batch path through a
    // qualified non-baseline decode-block implementation rather than weakening
    // the scheduler's eligibility rule for this test.
    air::ExecutionConfig execution;
    execution.cuda_decode_block_linear =
        air::QuantizedLinearExecutionKind::dense_f32_cublas;

    auto service = air::InferenceService::create(
        model,
        air::BackendPreference::cuda,
        0,
        scheduler,
        {},
        {},
        execution);
    if (!service) {
        std::cerr << "detailed CUDA graph service failed: "
                  << service.status().message() << '\n';
        return 1;
    }

    air::InferenceRequest request;
    request.prompt = "aa";
    request.generation.max_new_tokens = 3U;
    request.generation.sampling.temperature = 0.0;

    const std::vector<air::InferenceRequest> cohort_requests(
        2U, request);
    auto cohort =
        service.value()->generate_cohort(cohort_requests);
    if (!cohort || cohort.value().size() != 2U) {
        std::cerr << "CUDA graph cohort execution failed";
        if (!cohort) {
            std::cerr << ": " << cohort.status().message();
        }
        std::cerr << '\n';
        return 1;
    }

    const auto after_cohort = service.value()->snapshot();
    if (after_cohort.planned_decode_block_linear_tactic !=
        "dense-f32-cublas") {
        std::cerr << "CUDA graph cohort did not retain the requested qualified decode tactic: "
                  << after_cohort.planned_decode_block_linear_tactic << '\n';
        return 1;
    }
    if (after_cohort.physical_prefill_batches == 0U) {
        std::cerr << "CUDA graph cohort did not exercise native prefill batching"
                  << " physical_prefill_batches="
                  << after_cohort.physical_prefill_batches
                  << " physical_prefill_max_sequences="
                  << after_cohort.physical_prefill_max_sequences << '\n';
        return 1;
    }
    if (after_cohort.native_decode_batches == 0U) {
        std::cerr << "CUDA graph cohort did not exercise native decode batching"
                  << " native_decode_batches="
                  << after_cohort.native_decode_batches
                  << " native_decode_sequences="
                  << after_cohort.native_decode_sequences
                  << " decode_block="
                  << after_cohort.planned_decode_block_linear_tactic
                  << " decode_output="
                  << after_cohort.planned_decode_output_linear_tactic << '\n';
        return 1;
    }

    auto single = service.value()->generate(request);
    if (!single) {
        std::cerr << "CUDA graph single request failed: "
                  << single.status().message() << '\n';
        return 1;
    }

    const auto graphs =
        service.value()->execution_graph_timeline(512U);
    if (graphs.level != air::ExecutionObservationLevel::detailed ||
        graphs.topology_status != "ready" ||
        graphs.topology_fingerprint.empty() ||
        graphs.derivation_failures != 0U) {
        std::cerr << "CUDA graph observation topology/derivation state is invalid\n";
        return 1;
    }

    bool saw_single_prefill_concordant = false;
    bool saw_single_decode_concordant = false;
    bool saw_batch_prefill_incomplete = false;
    bool saw_batch_decode_incomplete = false;

    for (const auto& observation : graphs.observations) {
        if (!observation.graph ||
            observation.graph->backend() != air::BackendKind::cuda) {
            continue;
        }

        switch (observation.graph->invocation().kind) {
        case air::PhysicalInvocationKind::prefill_single:
            if (observation.evidence_status ==
                    air::ExecutionGraphEvidenceStatus::concordant &&
                observation.planned_transfer_regions ==
                    observation.matched_transfer_regions &&
                observation.planned_synchronization_regions ==
                    observation.matched_synchronization_regions) {
                saw_single_prefill_concordant = true;
            }
            break;
        case air::PhysicalInvocationKind::decode_single:
            if (observation.evidence_status ==
                    air::ExecutionGraphEvidenceStatus::concordant &&
                observation.planned_transfer_regions ==
                    observation.matched_transfer_regions &&
                observation.planned_synchronization_regions ==
                    observation.matched_synchronization_regions) {
                saw_single_decode_concordant = true;
            }
            break;
        case air::PhysicalInvocationKind::prefill_native_batch:
            if (observation.evidence_status ==
                    air::ExecutionGraphEvidenceStatus::incomplete &&
                observation.planned_transfer_regions >
                    observation.matched_transfer_regions &&
                observation.unexpected_transfer_spans == 0U &&
                observation.unexpected_synchronization_spans == 0U) {
                saw_batch_prefill_incomplete = true;
            }
            break;
        case air::PhysicalInvocationKind::decode_native_greedy_batch:
            if (observation.evidence_status ==
                    air::ExecutionGraphEvidenceStatus::incomplete &&
                observation.planned_transfer_regions >
                    observation.matched_transfer_regions &&
                observation.unexpected_transfer_spans == 0U &&
                observation.unexpected_synchronization_spans == 0U) {
                saw_batch_decode_incomplete = true;
            }
            break;
        }
    }

    if (!saw_single_prefill_concordant ||
        !saw_single_decode_concordant) {
        std::cerr << "single CUDA graph evidence was not concordant\n";
        return 1;
    }
    if (!saw_batch_prefill_incomplete ||
        !saw_batch_decode_incomplete) {
        std::cerr << "native CUDA batch graph did not preserve known shared-correlation evidence gaps\n";
        return 1;
    }

    std::cout << "CUDA ExecutionGraph planned/observed concordance characterization passed\n";
    return 0;
}

} // namespace

int main() {
#if AIR_HAS_CUDA
    if (!air::cuda_compiled()) {
        std::cerr << "AIR_HAS_CUDA is true but cuda_compiled() is false\n";
        return 1;
    }

    auto devices = air::cuda_devices();
    if (!devices) {
        // A CUDA build may run on a machine without an NVIDIA device/driver.
        // The compiled backend contract remains testable in that environment.
        std::cout << "CUDA backend compiled; runtime device query unavailable: "
                  << devices.status().message() << '\n';
        return 0;
    }

    std::cout << "CUDA backend compiled; devices="
              << devices.value().size() << '\n';
    if (devices.value().empty()) {
        std::cout << "CUDA backend compiled; no runtime device available for parity test\n";
        return 0;
    }

    const int capability = verify_cuda_operation_capabilities();
    if (capability != 0) return capability;

    const int parity = verify_cuda_reference_parity();
    if (parity != 0) return parity;

    const int graph_observation =
        verify_cuda_execution_graph_observation();
    if (graph_observation != 0) return graph_observation;

    return verify_cuda_alias_semantics();
#else
    if (air::cuda_compiled()) {
        std::cerr << "CPU-only build incorrectly reports compiled CUDA support\n";
        return 1;
    }

    auto devices = air::cuda_devices();
    if (devices || devices.status().code() != air::ErrorCode::unsupported) {
        std::cerr << "CPU-only CUDA device query must fail explicitly as unsupported\n";
        return 1;
    }

    std::cout << "CUDA-disabled contract behaves explicitly\n";
    return 0;
#endif
}
