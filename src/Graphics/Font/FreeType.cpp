//
// Created by Monika on 24.05.2023.
//

#include <Graphics/Font/FreeType.h>

namespace SR_GRAPH_NS {
    inline bool GetMonoPixel(const FT_Bitmap& bmp, int x, int y) {
        const uint8_t* row = bmp.buffer + y * bmp.pitch;
        uint8_t byte = row[x >> 3];
        uint8_t mask = 0x80 >> (x & 7);
        return (byte & mask) != 0;
    }

    void EDT_1D(const float* f, float* d, int n) {
        SR_TRACY_ZONE;

        SR_THREAD_LOCAL static std::vector<int> v;
        SR_THREAD_LOCAL static std::vector<float> z;

        v.clear();
        z.clear();

        z.resize(n + 1);
        v.resize(n);

        int k = 0;
        v[0] = 0;
        z[0] = -FLT_MAX;
        z[1] = FLT_MAX;

        for (int q = 1; q < n; q++) {
            float s;
            do {
                int vk = v[k];
                s = ((f[q] + q * q) - (f[vk] + vk*vk)) / (2 * q - 2 * vk);
                if (s <= z[k]) k--;
            } while (s <= z[k]);

            k++;
            v[k] = q;
            z[k] = s;
            z[k+1] = FLT_MAX;
        }

        k = 0;
        for (int q = 0; q < n; q++) {
            while (z[k+1] < q) k++;
            int vk = v[k];
            d[q] = (q - vk) * (q - vk) + f[vk];
        }
    }

    void EDT_2D(float* data, int w, int h) {
        SR_TRACY_ZONE;

        SR_THREAD_LOCAL static std::vector<float> tmp;
        tmp.clear();
        tmp.resize(std::max(w, h));

        // vertical
        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++)
                tmp[y] = data[y*w + x];

            EDT_1D(tmp.data(), tmp.data(), h);

            for (int y = 0; y < h; y++)
                data[y*w + x] = tmp[y];
        }

        // horizontal
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++)
                tmp[x] = data[y*w + x];

            EDT_1D(tmp.data(), tmp.data(), w);

            for (int x = 0; x < w; x++)
                data[y*w + x] = sqrtf(tmp[x]);
        }
    }

    void FreeTypeGenerateSDF(const FT_Bitmap& bmp, SR_HTYPES_NS::FastMemoryArray<float>& out, uint32_t width, uint32_t height, float_t range) {
        SR_TRACY_ZONE;
        out.resize(width * height);

        SR_THREAD_LOCAL static std::vector<float> inside;
        SR_THREAD_LOCAL static std::vector<float> outside;

        inside.clear();
        outside.clear();

        inside.resize(width * height);
        outside.resize(width * height);

        // 1. binary map
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bool isInside = GetMonoPixel(bmp, x, y);

                inside[y * width + x]  = isInside ? 0.0f : FLT_MAX;
                outside[y * width + x] = isInside ? FLT_MAX : 0.0f;
            }
        }

        // 2. distance transform
        EDT_2D(inside.data(), width, height);
        EDT_2D(outside.data(), width, height);

        // 3. signed distance
        for (int i = 0; i < width * height; i++) {
            float dist = outside[i] - inside[i];
            // нормализация
            dist = std::clamp(dist / range, -1.0f, 1.0f);
            out[i] = 0.5f + 0.5f * dist;
        }
    }
}