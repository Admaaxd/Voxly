#include "World.h"

World::World() : threadPool(std::thread::hardware_concurrency()) {
    for (int16_t x = -renderDistance; x <= renderDistance; ++x) {
        for (int16_t z = -renderDistance; z <= renderDistance; ++z) {
            glm::vec3 position(x * (CHUNK_SIZE - 1), 0, z * (CHUNK_SIZE - 1));
            std::pair<int16_t, int16_t> chunkCoord = { x, z };
            chunks.emplace(chunkCoord, Chunk(position, chunkCoord, this));

            threadPool.enqueue([this, chunkCoord, position]() {
                ChunkMeshData data = generateChunkMeshData(chunkCoord, position);
                std::lock_guard<std::mutex> lock(meshQueueMutex);
                meshUploadQueue.push(std::move(data));
            });
        }
    }
}

void World::processMeshUploads() {
    std::lock_guard<std::mutex> lock(meshQueueMutex);
    while (!meshUploadQueue.empty()) {
        ChunkMeshData mesh = std::move(meshUploadQueue.front());
        meshUploadQueue.pop();

        Chunk* chunk = getChunkPtr(mesh.coord);
        if (chunk) {
            chunk->uploadMeshFromThread(mesh);
        }
    }
}


void World::render(shader& mainShader) {
    for (auto& [key, chunk] : chunks) {
        mainShader.setVec3("chunkOffset", chunk.getOffset());
        chunk.render(mainShader);
    }
}

std::vector<std::reference_wrapper<Chunk>> World::getChunks()
{
    std::vector<std::reference_wrapper<Chunk>> chunkList;
    for (auto& [key, chunk] : chunks) {
        chunkList.push_back(chunk);
    }
    return chunkList;
}

bool World::hasChunk(const std::pair<int, int>& cpos) {
    std::lock_guard<std::mutex> lock(chunksMutex);
    return (chunks.find(cpos) != chunks.end());
}

Chunk* World::getChunkPtr(const std::pair<int, int>& cpos) {
    std::lock_guard<std::mutex> lock(chunksMutex);
    auto it = chunks.find(cpos);
    if (it != chunks.end()) {
        return &it->second;
    }
    return nullptr;
}

void World::loadChunk(int16_t x, int16_t z) {
    std::pair<int, int> chunkCoord = { x, z };

    if (hasChunk(chunkCoord)) {
        return;
    }

    glm::vec3 position(x * (CHUNK_SIZE - 1), 0, z * (CHUNK_SIZE - 1));

    chunks.emplace(chunkCoord, Chunk(position, chunkCoord, this));

    threadPool.enqueue([this, chunkCoord, position]() {
        ChunkMeshData data = generateChunkMeshData(chunkCoord, position);

        std::lock_guard<std::mutex> lock(meshQueueMutex);
        meshUploadQueue.push(std::move(data));
        });
}

void World::unloadChunk(int16_t x, int16_t z) {
    std::pair<int, int> chunkCoord = { x, z };

    auto it = chunks.find(chunkCoord);
    if (it != chunks.end()) {
        it->second.cleanupOpenGLResources();
        chunks.erase(it);
    }
}

void World::updateChunks(glm::vec3 playerPosition) {
    int16_t playerChunkX = static_cast<int16_t>(std::floor(playerPosition.x / CHUNK_SIZE));
    int16_t playerChunkZ = static_cast<int16_t>(std::floor(playerPosition.z / CHUNK_SIZE));

    std::unordered_set<std::pair<int, int>, hash_pair> activeChunks;

    for (int16_t x = -renderDistance; x <= renderDistance; ++x) {
        for (int16_t z = -renderDistance; z <= renderDistance; ++z) {
            std::pair<int, int> chunkCoord = { playerChunkX + x, playerChunkZ + z };
            activeChunks.insert(chunkCoord);

            if (!hasChunk(chunkCoord)) {
                std::lock_guard<std::mutex> lock(pendingMutex);
                if (std::find(pendingChunks.begin(), pendingChunks.end(), chunkCoord) == pendingChunks.end()) {
                    pendingChunks.push_back(chunkCoord);
                }
            }
        }
    }

    auto now = std::chrono::steady_clock::now();
    if (now - lastChunkLoadTime >= loadDelay) {
        std::lock_guard<std::mutex> lock(pendingMutex);
        if (!pendingChunks.empty()) {
            auto coord = pendingChunks.front();
            pendingChunks.pop_front();
            loadChunk(coord.first, coord.second);
            lastChunkLoadTime = now;
        }
    }

    std::vector<std::pair<int, int>> chunksToUnload;
    {
        std::lock_guard<std::mutex> lock(chunksMutex);
        for (const auto& [chunkCoord, chunk] : chunks) {
            if (activeChunks.find(chunkCoord) == activeChunks.end()) {
                chunksToUnload.push_back(chunkCoord);
            }
        }
    }

    for (const auto& chunkCoord : chunksToUnload) {
        unloadChunk(chunkCoord.first, chunkCoord.second);
    }
}

ChunkMeshData World::generateChunkMeshData(std::pair<int16_t, int16_t> chunkCoord, glm::vec3 position) {
    ChunkMeshData data;
    data.coord = chunkCoord;
    data.offset = position;
    data.blocks.resize(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE, BlockType::AIR);

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalOctaves(1);
    noise.SetFractalLacunarity(1.0f);
    noise.SetFractalGain(0.5f);
    constexpr GLfloat noiseScale = 0.9f;

    // Generate block data
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            float worldX = position.x + x;
            float worldZ = position.z + z;
            float noiseValue = noise.GetNoise(worldX * noiseScale, worldZ * noiseScale);
            int height = static_cast<int>((noiseValue + 1.0f) * 0.5f * CHUNK_HEIGHT);
            for (int y = 0; y <= height; ++y) {
                data.blocks[x + CHUNK_SIZE * (y + CHUNK_HEIGHT * z)] = BlockType::SOLID;
            }
        }
    }

    // Generate mesh
    GLuint indexOffset = 0;
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                int idx = x + CHUNK_SIZE * (y + CHUNK_HEIGHT * z);
                if (data.blocks[idx] == BlockType::AIR) continue;

                glm::vec3 blockPos(x, y, z);

                // A helper that only works for blocks inside the same chunk
                auto isFaceVisibleInternal = [&](int nx, int ny, int nz) -> bool {
                    if (nx < 0 || ny < 0 || nz < 0 || nx >= CHUNK_SIZE || ny >= CHUNK_HEIGHT || nz >= CHUNK_SIZE)
                        return true;
                    return data.blocks[nx + CHUNK_SIZE * (ny + CHUNK_HEIGHT * nz)] == BlockType::AIR;
                };

                if (isFaceVisibleInternal(x - 1, y, z))
                    AddFaceToMesh(data.vertices, data.indices, blockPos, LEFT_FACE, indexOffset);
                if (isFaceVisibleInternal(x + 1, y, z))
                    AddFaceToMesh(data.vertices, data.indices, blockPos, RIGHT_FACE, indexOffset);
                if (isFaceVisibleInternal(x, y + 1, z))
                    AddFaceToMesh(data.vertices, data.indices, blockPos, TOP_FACE, indexOffset);
                if (isFaceVisibleInternal(x, y - 1, z))
                    AddFaceToMesh(data.vertices, data.indices, blockPos, BOTTOM_FACE, indexOffset);
                if (isFaceVisibleInternal(x, y, z + 1))
                    AddFaceToMesh(data.vertices, data.indices, blockPos, FRONT_FACE, indexOffset);
                if (isFaceVisibleInternal(x, y, z - 1))
                    AddFaceToMesh(data.vertices, data.indices, blockPos, BACK_FACE, indexOffset);
            }
        }
    }

    return data;
}