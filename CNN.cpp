#include <iostream>
#include <vector>
#include <memory>
#include <numeric>
#include <random>

typedef std::vector<float_t> vec_t;

using namespace std;

template <typename T>
    using Ref = shared_ptr<T>;

float sigmoid_fast(float x) {
    return x / (1 + abs(x));
}

float sigmoid(float x) {
    return 1 / (1 + exp(-x));
}

float df(float x) {
    return x * (1 - x);
}

vec_t df(const vec_t& y, size_t i) { 
    vec_t v(y.size(), 0); 
    v[i] = df(y[i]); 
    return v; 
}

float activation(float x) {
    return sigmoid(x);
}

float dot(vec_t a, vec_t b) {
    if(a.size() != b.size()) { throw std::runtime_error("different sizes"); }

    return inner_product(a.begin(), a.end(), b.begin(), 0.0);
}

float cost(vec_t a, vec_t b) {
    if(a.size() != b.size()) { throw std::runtime_error("different sizes"); }

    vec_t diff;
    diff.resize(a.size());

    transform(a.begin(), a.end(), b.begin(), diff.begin(), [&](float x, float y) {
        return (x - y) * (x - y);
    });

    return accumulate(diff.begin(), diff.end(), 0.0);
}

vec_t error(vec_t a, vec_t b) {
    if(a.size() != b.size()) { throw std::runtime_error("different sizes"); }

    vec_t error;
    error.resize(a.size());

    transform(a.begin(), a.end(), b.begin(), error.begin(), [&](float x, float y) {
        return x - y;
    });
}

float sum(vec_t a) {
    return accumulate(a.begin(), a.end(), 0.0);
}

class Layer {
public:
    Layer* prev;
    Layer* next;

    vec_t values;
    vec_t biases;
    vector<vec_t> weights;

    uint32_t in_size;
    uint32_t out_size;
    uint32_t weight_size;
    uint32_t bias_size;

    Layer(uint32_t in_size, uint32_t out_size, uint32_t weight_size, uint32_t bias_size) : in_size{in_size}, out_size{out_size}, weight_size{weight_size}, bias_size{bias_size}, prev{ nullptr }, next{ nullptr } {
        values.resize(in_size);
        biases.resize(in_size);
    }

    void init_weights() {
        if(!next) {
            return;
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-1, 1);

        weights.resize(out_size);

        for(size_t i = 0; i < out_size; i++) {
            weights[i].resize(in_size);
            for(size_t j = 0; j < in_size; j++) {
                float r = dis(gen);
                weights[i][j] = r;
            }
        }
    }

    void print_weights() {
        if(!next) {
            return;
        }

        for(size_t i = 0; i < in_size; i++) {
            for(size_t j = 0; j < out_size; j++) {
                printf("%f - %d %d", weights[i][j], i, j);
            }
        }
    }

    void add_tail(shared_ptr<Layer> layer) {
        next = layer.get();
        layer->prev = this;
    }

    void forward_propogate() {
        if(!next) return;


        for(size_t i = 0; i < out_size; i++) {
            vec_t w = weights[i];
            float d = dot(w, values) + next->biases[i];
            next->values[i] = activation(d);
        }

        next->forward_propogate();
    }

    void back_propogate(vec_t delta) {
        if(!prev) return;

        vec_t prev_delta;
        prev_delta.resize(prev->in_size);

        for(size_t i = 0; i < in_size; i++) {
            vec_t& w = prev->weights[i];

            for(size_t j = 0; j < w.size(); j++) {
                w[j] = w[j] + (0.1f * delta[i] * prev->values[j]);

                prev_delta[j] = prev->values[j] * (1.0f - prev->values[j]) * w[j] * delta[i];
            }
        }

        

        prev->back_propogate(prev_delta);
    }
};

class CNN {
    public:
        vector<Ref<Layer>> layers;

        void forward_propogate() {
            head()->forward_propogate();
        }

        void back_propogate(vec_t labels) {
            vec_t delta(tail()->in_size);
            for(size_t i = 0; i < tail()->in_size; i++) {
                delta[i] = tail()->values[i] * (1.0f - tail()->values[i]) * (labels[i] - tail()->values[i]);
            }

            tail()->back_propogate(delta);
        }

        void init() {
            for(auto& layer : layers) layer->init_weights();
        }

        void print_output() {
            for(float v : tail()->values) {
                printf("%f\n", v);
            }
        }

        vec_t& get_output() {
            return tail()->values;
        }

        void add(shared_ptr<Layer> layer) {
            if(tail()) tail()->add_tail(layer);
            layers.push_back(layer);
        }

        bool empty() const { return layers.size() == 0; }

        Layer* head() const { return empty() ? 0 : layers[0].get(); }

        Layer* tail() const { return empty() ? 0 : layers[layers.size() - 1].get(); }
};

int main() {
    Ref<Layer> input   = make_shared<Layer>(2, 4, 0, 0);
    Ref<Layer> hidden  = make_shared<Layer>(4, 1, 0, 0);
    Ref<Layer> output  = make_shared<Layer>(1, 1, 0, 0);
    CNN cnn {};
    cnn.add(input);
    cnn.add(hidden);
    cnn.add(output);
    cnn.init();

    vector<vec_t> training;
    vector<vec_t> labels;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);

    for(int i = 0; i < 100000; i++) {
        int a = round(dis(gen));
        int b = round(dis(gen));

        bool res = ~(a | b) & 0b1;

        training.push_back({ (float) a, (float) b });
        labels.push_back({ (float) res });
    }
    
    cout << "Training..." << endl;

    for(int i = 0; i < 100000; i++) {
        input->values = training[i];

        cnn.forward_propogate();
        cnn.back_propogate(labels[i]);
    }

    cout << "Results!" << endl;

    for(int i = 0; i < 20; i++) {
        int a = round(dis(gen));
        int b = round(dis(gen));

        bool res = a & b;

        input->values = { (float) a, (float) b };

        cnn.forward_propogate();

        printf("%d NOR %d = %f\n", a, b, cnn.get_output()[0]);
    }

    int a, b;

    cout << "Try it yourself!" << endl;
    cin >> a >> b;

    input->values = { (float)a, (float)b };

    cnn.forward_propogate();

    printf("%d NOR %d = %f\n", a, b, cnn.get_output()[0]);
}