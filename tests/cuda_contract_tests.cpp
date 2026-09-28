#include "air/cuda.hpp"
#include "air/reference.hpp"
#include "air/storage.hpp"

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
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

bool near(float left, float right, float tolerance = 1.0e-4F) {
    return std::fabs(left - right) <= tolerance;
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

    air::ReferenceKvCache reference_cache(1, 1, 2, 8);
    auto cuda_cache = cuda.value()->create_kv_cache();
    if (!cuda_cache) {
        std::cerr << "CUDA KV fixture failed: "
                  << cuda_cache.status().message() << '\n';
        return 1;
    }

    auto expected = reference.value()->step(2, reference_cache);
    if (!expected) {
        std::cerr << "reference step failed: "
                  << expected.status().message() << '\n';
        return 1;
    }

    auto actual = cuda.value()->step(2, *cuda_cache.value());
    if (!actual) {
        std::cerr << "CUDA step failed: "
                  << actual.status().message() << '\n';
        return 1;
    }

    if (actual.value().size() != expected.value().size()) {
        std::cerr << "CUDA/reference logit width mismatch\n";
        return 1;
    }

    for (std::size_t i = 0; i < expected.value().size(); ++i) {
        if (!near(actual.value()[i], expected.value()[i])) {
            std::cerr << "CUDA/reference logit mismatch at index " << i
                      << ": cuda=" << actual.value()[i]
                      << " reference=" << expected.value()[i] << '\n';
            return 1;
        }
    }

    std::cout << "CUDA/reference semantic-binding parity passed\n";
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

    return verify_cuda_reference_parity();
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
