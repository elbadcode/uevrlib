#pragma once

#include "API.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>

// adapted from lua uevrlib, mostly 
// AI generated as an experiment to see if this would even be viable. Use at your own risk. Compiler caught nothing wrong but its untested

namespace uevr_utils {

    using namespace uevr;

    // Simple log levels
    enum class LogLevel {
        Off = 0,
        Critical = 1,
        Error = 2,
        Warning = 3,
        Info = 4,
        Debug = 5,
        Trace = 6,
        Ignore = 99,
    };

    // Vector/Quat/Rotator helpers
    inline std::unique_ptr<API::UObject> vector2(float x, float y) {
        auto vec2_class = API::get()->find_uobject<API::UClass>(L"ScriptStruct /Script/CoreUObject.Vector2D");
        if (!vec2_class) return nullptr;
        auto vec2 = std::unique_ptr<API::UObject>(API::get()->spawn_object(vec2_class, nullptr));
        if (vec2) {
            vec2->get_property<float>(L"X") = x;
            vec2->get_property<float>(L"Y") = y;
        }
        return vec2;
    }

    inline std::unique_ptr<API::UObject> vector3(float x, float y, float z) {
        auto vec3_class = API::get()->find_uobject<API::UClass>(L"ScriptStruct /Script/CoreUObject.Vector");
        if (!vec3_class) return nullptr;
        auto vec3 = std::unique_ptr<API::UObject>(API::get()->spawn_object(vec3_class, nullptr));
        if (vec3) {
            vec3->get_property<float>(L"X") = x;
            vec3->get_property<float>(L"Y") = y;
            vec3->get_property<float>(L"Z") = z;
        }
        return vec3;
    }

    inline std::unique_ptr<API::UObject> quat(float x, float y, float z, float w) {
        auto quat_class = API::get()->find_uobject<API::UClass>(L"ScriptStruct /Script/CoreUObject.Quat");
        if (!quat_class) return nullptr;
        auto q = std::unique_ptr<API::UObject>(API::get()->spawn_object(quat_class, nullptr));
        if (q) {
            q->get_property<float>(L"X") = x;
            q->get_property<float>(L"Y") = y;
            q->get_property<float>(L"Z") = z;
            q->get_property<float>(L"W") = w;
        }
        return q;
    }

    inline std::unique_ptr<API::UObject> rotator(float pitch, float yaw, float roll) {
        auto rot_class = API::get()->find_uobject<API::UClass>(L"ScriptStruct /Script/CoreUObject.Rotator");
        if (!rot_class) return nullptr;
        auto rot = std::unique_ptr<API::UObject>(API::get()->spawn_object(rot_class, nullptr));
        if (rot) {
            rot->get_property<float>(L"Pitch") = pitch;
            rot->get_property<float>(L"Yaw") = yaw;
            rot->get_property<float>(L"Roll") = roll;
        }
        return rot;
    }

    // Transform helper
    inline std::unique_ptr<API::UObject> get_transform(
        const std::tuple<float, float, float>& position = { 0, 0, 0 },
        const std::tuple<float, float, float, float>& rotation = { 0, 0, 0, 1 },
        const std::tuple<float, float, float>& scale = { 1, 1, 1 }
    ) {
        auto transform_class = API::get()->find_uobject<API::UClass>(L"ScriptStruct /Script/CoreUObject.Transform");
        if (!transform_class) return nullptr;
        auto transform = std::unique_ptr<API::UObject>(API::get()->spawn_object(transform_class, nullptr));
        if (transform) {
            auto [px, py, pz] = position;
            auto [rx, ry, rz, rw] = rotation;
            auto [sx, sy, sz] = scale;
            // Set Translation
            auto translation = vector3(px, py, pz);
            if (translation) {
                transform->get_property<API::UObject*>(L"Translation") = translation.get();
            }
            // Set Rotation
            auto rot = quat(rx, ry, rz, rw);
            if (rot) {
                transform->get_property<API::UObject*>(L"Rotation") = rot.get();
            }
            // Set Scale3D
            auto scale3d = vector3(sx, sy, sz);
            if (scale3d) {
                transform->get_property<API::UObject*>(L"Scale3D") = scale3d.get();
            }
        }
        return transform;
    }

    // World and actor helpers
    inline API::UWorld* get_world() {
        auto engine = API::get()->get_engine();
        if (!engine) return nullptr;
        auto viewport = engine->get_property<API::UObject*>(L"GameViewport");
        if (!viewport) return nullptr;
        return viewport->get_property<API::UWorld*>(L"World");
    }

    inline API::UObject* spawn_actor(
        API::UObject* transform,
        int collisionMethod = 1,
        API::UObject* owner = nullptr
    ) {
        auto engine = API::gaet()->get_engine();
        if (!engine) return nullptr;
        auto viewport = engine->get_property<API::UObject*>(L"GameViewport");
        if (!viewport) return nullptr;
        auto world = viewport->get_property<API::UWorld*>(L"World");
        if (!world) return nullptr;

        auto statics = API::get()->find_uobject<API::UObject>(L"Class /Script/Engine.GameplayStatics");
        auto actor_class = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.Actor");
        if (!statics || !actor_class) return nullptr;

        // BeginDeferredActorSpawnFromClass
        auto begin_fn = statics->get_class()->find_function(L"BeginDeferredActorSpawnFromClass");
        struct {
            API::UWorld* WorldContext;
            API::UClass* ActorClass;
            API::UObject* Transform;
            int CollisionHandlingOverride;
            API::UObject* Owner;
            API::UObject* ReturnValue;
        } params{ world, actor_class, transform, collisionMethod, owner, nullptr };
        statics->process_event(begin_fn, &params);
        auto actor = params.ReturnValue;
        if (!actor) return nullptr;

        // FinishSpawningActor
        auto finish_fn = statics->get_class()->find_function(L"FinishSpawningActor");
        struct {
            API::UObject* Actor;
            API::UObject* Transform;
            API::UObject* ReturnValue;
        } finish_params{ actor, transform, nullptr };
        statics->process_event(finish_fn, &finish_params);

        return finish_params.ReturnValue;
    }

    // Validate UObject
    inline bool validate_object(API::UObject* object) {
        if (!object) return false;
        return API::UObjectHook::exists(object);
    }

    // Find required UObject
    inline API::UObject* find_required_object(const std::wstring& name) {
        auto obj = API::get()->find_uobject<API::UObject>(name);
        if (!obj) throw std::runtime_error("Cannot find object: " + std::string(name.begin(), name.end()));
        return obj;
    }

    // Get class with cache
    class ClassCache {
    public:
        API::UClass* get_class(const std::wstring& name, bool clearCache = false) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (clearCache || cache_.find(name) == cache_.end()) {
                cache_[name] = API::get()->find_uobject<API::UClass>(name);
            }
            return cache_[name];
        }
    private:
        std::unordered_map<std::wstring, API::UClass*> cache_;
        std::mutex mutex_;
    };

    // Example: set cvar int
    inline void set_cvar_int(const std::wstring& cvar, int value) {
        auto console_manager = API::get()->get_console_manager();
        if (!console_manager) return;
        auto var = console_manager->find_variable(cvar);
        if (var) var->set(value);
    }

    // Example: hook function
    inline bool hook_function(
        const std::wstring& class_name,
        const std::wstring& function_name,
        bool native,
        API::UFunction::UEVR_UFunction_CPPPreNative prefn,
        API::UFunction::UEVR_UFunction_CPPPostNative postfn,
        bool dbgout = false
    ) {
        if (dbgout) API::get()->log_info("Hook_function for %ls %ls", class_name.c_str(), function_name.c_str());
        auto class_obj = API::get()->find_uobject<API::UClass>(class_name);
        if (!class_obj) return false;
        auto class_fn = class_obj->find_function(function_name);
        if (!class_fn) return false;
        if (native) {
            class_fn->set_function_flags(class_fn->get_function_flags() | 0x400);
            if (dbgout) API::get()->log_info("hook_function: set native flag");
        }
        bool result = class_fn->hook_ptr(prefn, postfn);
        if (dbgout) API::get()->log_info("hook_function: set function hook");
        return result;
    }

    // Helper: Split string on last period
    inline std::pair<std::wstring, std::wstring> splitOnLastPeriod(const std::wstring& input) {
        size_t pos = input.find_last_of(L'.');
        if (pos == std::wstring::npos) return { input, L"" };
        return { input.substr(0, pos), input.substr(pos + 1) };
    }

    // Find instance of a class by full or short name
    inline uevr::API::UObject* find_instance_of(const std::wstring& className, const std::wstring& objectName) {
        auto classObj = uevr::API::get()->find_uobject<uevr::API::UClass>(className);
        if (!classObj) return nullptr;
        auto instances = classObj->get_objects_matching(true);
        bool isShortName = objectName.find(L'.') == std::wstring::npos;
        for (auto* instance : instances) {
            std::wstring fullName = instance->get_full_name();
            if (isShortName) {
                auto [before, after] = splitOnLastPeriod(fullName);
                if (!after.empty() && after == objectName) return instance;
            }
            else {
                if (fullName == objectName) return instance;
            }
        }
        return nullptr;
    }

    // Find all instances of a class
    inline std::vector<uevr::API::UObject*> find_all_of(const std::wstring& className, bool includeDefault = false) {
        auto classObj = uevr::API::get()->find_uobject<uevr::API::UClass>(className);
        if (!classObj) return {};
        return classObj->get_objects_matching(includeDefault);
    }

    // Find first instance of a class
    inline uevr::API::UObject* find_first_of(const std::wstring& className, bool includeDefault = false) {
        auto classObj = uevr::API::get()->find_uobject<uevr::API::UClass>(className);
        if (!classObj) return nullptr;
        return classObj->get_first_object_matching(includeDefault);
    }

    // Find default instance (CDO) of a class
    inline uevr::API::UObject* find_default_instance(const std::wstring& className) {
        auto classObj = uevr::API::get()->find_uobject<uevr::API::UClass>(className);
        if (!classObj) return nullptr;
        return classObj->get_class_default_object();
    }

    // Get FName from string
    inline uevr::API::FName* fname_from_string(const std::wstring& str) {
        // This assumes you have a string-to-FName conversion function in your API
        // If not, you may need to create one using the FName constructor
        return new uevr::API::FName(str, uevr::API::FName::EFindName::Add);
    }

    // Color helpers
    inline uevr::API::UObject* color_from_rgba(float r, float g, float b, float a) {
        auto colorClass = uevr::API::get()->find_uobject<uevr::API::UClass>(L"ScriptStruct /Script/CoreUObject.LinearColor");
        auto color = uevr::API::get()->spawn_object(colorClass, nullptr);
        if (color) {
            color->get_property<float>(L"R") = r;
            color->get_property<float>(L"G") = g;
            color->get_property<float>(L"B") = b;
            color->get_property<float>(L"A") = a;
        }
        return color;
    }
    inline uevr::API::UObject* color_from_rgba_int(int r, int g, int b, int a) {
        auto colorClass = uevr::API::get()->find_uobject<uevr::API::UClass>(L"ScriptStruct /Script/CoreUObject.Color");
        auto color = uevr::API::get()->spawn_object(colorClass, nullptr);
        if (color) {
            color->get_property<int>(L"R") = r;
            color->get_property<int>(L"G") = g;
            color->get_property<int>(L"B") = b;
            color->get_property<int>(L"A") = a;
        }
        return color;
    }

    // Copy materials from one component to another
    inline void copyMaterials(uevr::API::UObject* fromComponent, uevr::API::UObject* toComponent) {
        if (!fromComponent || !toComponent) return;
        auto getMaterialsFn = fromComponent->get_class()->find_function(L"GetMaterials");
        if (!getMaterialsFn) return;
        // This assumes GetMaterials returns a TArray<UObject*>
        struct { uevr::API::UObject* ReturnValue; } params{};
        fromComponent->process_event(getMaterialsFn, &params);
        auto materials = reinterpret_cast<uevr::API::TArray<uevr::API::UObject*>*>(params.ReturnValue);
        if (!materials) return;
        for (int i = 0; i < materials->count; ++i) {
            auto setMaterialFn = toComponent->get_class()->find_function(L"SetMaterial");
            if (setMaterialFn) {
                struct { int Index; uevr::API::UObject* Material; } setParams{ i, materials->data[i] };
                toComponent->process_event(setMaterialFn, &setParams);
            }
        }
    }

    // Get child component by partial name
    inline uevr::API::UObject* getChildComponent(uevr::API::UObject* parent, const std::wstring& name) {
        if (!parent) return nullptr;
        auto children = parent->get_property<uevr::API::TArray<uevr::API::UObject*>>(L"AttachChildren");
        for (int i = 0; i < children.count; ++i) {
            auto* child = children.data[i];
            if (child && child->get_full_name().find(name) != std::wstring::npos) {
                return child;
            }
        }
        return nullptr;
    }

    // Set cvar int (already in previous translation, but included for completeness)
    inline void set_cvar_int(const std::wstring& cvar, int value) {
        auto console_manager = uevr::API::get()->get_console_manager();
        if (!console_manager) return;
        auto var = console_manager->find_variable(cvar);
        if (var) var->set(value);
    }

    // Print all instance names of a class
    inline void PrintInstanceNames(const std::wstring& class_to_search) {
        auto classObj = uevr::API::get()->find_uobject<uevr::API::UClass>(class_to_search);
        if (!classObj) {
            uevr::API::get()->log_info("%ls was not found", class_to_search.c_str());
            return;
        }
        auto instances = classObj->get_objects_matching(false);
        for (size_t i = 0; i < instances.size(); ++i) {
            auto* instance = instances[i];
            uevr::API::get()->log_info("%zu %ls %ls", i, instance->get_fname()->to_string().c_str(), instance->get_full_name().c_str());
        }
    }



class ControllerManager {
public:
    enum ControllerID { Left = 0, Right = 1, HMD = 2 };

    struct Controller {
        std::shared_ptr<API::UObject> actor;
        std::shared_ptr<API::UObject> component;
    };

    ControllerManager() { resetControllers(); }

    void onLevelChange() {
        resetMotionControllers();
        resetControllers();
    }

    void resetControllers() {
        for (int i = 0; i < 3; ++i) controllers_[i] = nullptr;
    }

    void resetMotionControllers() {
        // If you have a UObjectHook static method to remove all states, call it here
        // API::UObjectHook::remove_all_motion_controller_states();
    }

    std::shared_ptr<API::UObject> createController(int controllerID) {
        if (controllerID == HMD) return createHMDController();
        if (controllerExists(controllerID)) return controllers_[controllerID]->component;

        auto actor = std::shared_ptr<API::UObject>(spawn_actor(get_transform(), 1, nullptr));
        auto compClass = get_class(L"Class /Script/HeadMountedDisplay.MotionControllerComponent");
        auto addCompFn = actor->get_class()->find_function(L"AddComponentByClass");
        struct {
            API::UClass* Class;
            bool ManualAttachment;
            API::UObject* Transform;
            bool DeferredFinish;
            API::UObject* ReturnValue;
        } params{ compClass, true, get_transform().get(), false, nullptr };
        actor->process_event(addCompFn, &params);
        auto component = std::shared_ptr<API::UObject>(params.ReturnValue);

        // Set properties as needed (MotionSource, Hand, etc.)
        controllers_[controllerID] = std::make_shared<Controller>(Controller{ actor, component });
        return component;
    }

    std::shared_ptr<API::UObject> createHMDController() {
        if (controllerExists(HMD)) return controllers_[HMD]->component;
        auto actor = std::shared_ptr<API::UObject>(spawn_actor(get_transform(), 1, nullptr));
        auto compClass = get_class(L"Class /Script/Engine.SceneComponent");
        auto addCompFn = actor->get_class()->find_function(L"AddComponentByClass");
        struct {
            API::UClass* Class;
            bool ManualAttachment;
            API::UObject* Transform;
            bool DeferredFinish;
            API::UObject* ReturnValue;
        } params{ compClass, true, get_transform().get(), false, nullptr };
        actor->process_event(addCompFn, &params);
        auto component = std::shared_ptr<API::UObject>(params.ReturnValue);

        // Optionally set up HMD state here
        controllers_[HMD] = std::make_shared<Controller>(Controller{ actor, component });
        return component;
    }

    std::shared_ptr<API::UObject> getController(int controllerID) {
        if (controllerID == HMD) return controllers_[HMD] ? controllers_[HMD]->component : nullptr;
        return controllers_[controllerID] ? controllers_[controllerID]->component : nullptr;
    }

    bool controllerExists(int controllerID) {
        return controllers_[controllerID] && validate_object(controllers_[controllerID]->component.get());
    }

    void destroyController(int controllerID) {
        if (controllers_[controllerID]) {
            // Optionally destroy components/actor
            auto actor = controllers_[controllerID]->actor;
            if (actor) {
                auto destroyFn = actor->get_class()->find_function(L"K2_DestroyActor");
                if (destroyFn) {
                    actor->process_event(destroyFn, nullptr);
                }
            }
            controllers_[controllerID] = nullptr;
        }
    }

    void destroyControllers() {
        for (int i = 0; i < 3; ++i) destroyController(i);
        resetControllers();
    }

    bool attachComponentToController(int controllerID, API::UObject* childComponent, const std::wstring& socketName = L"", int attachType = 0, bool weld = false) {
        auto controller = getController(controllerID);
        if (!controller || !childComponent) return false;
        auto attachFn = childComponent->get_class()->find_function(L"K2_AttachTo");
        if (!attachFn) return false;
        struct {
            API::UObject* Parent;
            API::FName* SocketName;
            int AttachType;
            bool Weld;
        } params{ controller.get(), fname_from_string(socketName), attachType, weld };
        childComponent->process_event(attachFn, &params);
        return true;
    }

    std::unique_ptr<API::UObject> getControllerLocation(int controllerID) {
        auto controller = getController(controllerID);
        if (!controller) return nullptr;
        auto fn = controller->get_class()->find_function(L"K2_GetComponentLocation");
        if (!fn) return nullptr;
        struct { API::UObject* ReturnValue; } params{};
        controller->process_event(fn, &params);
        return std::unique_ptr<API::UObject>(params.ReturnValue);
    }

    std::unique_ptr<API::UObject> getControllerRotation(int controllerID) {
        auto controller = getController(controllerID);
        if (!controller) return nullptr;
        auto fn = controller->get_class()->find_function(L"K2_GetComponentRotation");
        if (!fn) return nullptr;
        struct { API::UObject* ReturnValue; } params{};
        controller->process_event(fn, &params);
        return std::unique_ptr<API::UObject>(params.ReturnValue);
    }

    std::unique_ptr<API::UObject> getControllerDirection(int controllerID) {
        auto controller = getController(controllerID);
        if (!controller) return nullptr;
        auto rot = getControllerRotation(controllerID);
        if (!rot) return nullptr;
        // Assuming you have a math library function for GetForwardVector
        auto mathLib = find_default_instance(L"Class /Script/Engine.KismetMathLibrary");
        auto fn = mathLib->get_class()->find_function(L"GetForwardVector");
        if (!fn) return nullptr;
        struct { API::UObject* Rotator; API::UObject* ReturnValue; } params{ rot.get(), nullptr };
        mathLib->process_event(fn, &params);
        return std::unique_ptr<API::UObject>(params.ReturnValue);
    }

private:
    std::shared_ptr<Controller> controllers_[3];
};

// -------------------- ANIMATION --------------------
// Log level for animation
static LogLevel currentLogLevel = LogLevel::Error;

inline void animation_setLogLevel(LogLevel val) {
    currentLogLevel = val;
}

inline void animation_print(const std::string& text, LogLevel logLevel = LogLevel::Debug) {
    if (logLevel <= currentLogLevel) {
        uevr_utils::print("[animation] " + text, logLevel);
    }
}

// Animation data structures
struct BoneAngles {
    float pitch, yaw, roll;
};

struct AnimationPose {
    std::unordered_map<std::wstring, BoneAngles> boneAngles;
};

struct AnimationDefinition {
    // positions[animName][val] = AnimationPose
    std::unordered_map<std::string, std::unordered_map<std::string, AnimationPose>> positions;
    // poses[poseID] = vector<pair<animName, val>>
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> poses;
    // initialTransform[animID][boneName] = {rotation, location, scale}
    std::unordered_map<std::string, std::unordered_map<std::wstring, std::unordered_map<std::string, std::vector<float>>>> initialTransform;
};

struct AnimationInstance {
    API::UObject* component = nullptr;
    std::shared_ptr<AnimationDefinition> definitions;
};

static std::unordered_map<std::string, AnimationInstance> animations;
static std::unordered_map<std::string, std::unordered_map<std::string, bool>> animStates;

// Poseable mesh creation
inline API::UObject* createPoseableComponent(API::UObject* skeletalMeshComponent, API::UObject* parent) {
    if (!skeletalMeshComponent) {
        animation_print("SkeletalMeshComponent was not valid in createPoseableComponent", LogLevel::Warning);
        return nullptr;
    }
    return uevr_utils::createPoseableMeshFromSkeletalMesh(skeletalMeshComponent, parent);
}

// Bone transform helpers
inline API::UObject* getRootBoneOfBone(API::UObject* skeletalMeshComponent, const std::wstring& boneName) {
    auto fName = fname_from_string(boneName);
    auto current = fName;
    while (skeletalMeshComponent->call<bool>(L"GetParentBone", current) != L"None") {
        current = skeletalMeshComponent->call<API::FName*>(L"GetParentBone", current);
    }
    return current;
}

// Animation logic
inline void animate(const std::string& animID, const std::string& animName, const std::string& val) {
    animation_print("Called animate with " + animID + " " + animName + " " + val, LogLevel::Info);
    auto it = animations.find(animID);
    if (it == animations.end()) return;
    auto& anim = it->second;
    if (!anim.component || !anim.definitions) {
        animation_print("Component was nil in animate", LogLevel::Warning);
        return;
    }
    auto posIt = anim.definitions->positions.find(animName);
    if (posIt == anim.definitions->positions.end()) return;
    auto valIt = posIt->second.find(val);
    if (valIt == posIt->second.end()) return;
    for (const auto& bonePair : valIt->second.boneAngles) {
        auto localRotator = uevr_utils::rotator(bonePair.second.pitch, bonePair.second.yaw, bonePair.second.roll);
        animation_print("Animating " + std::string(bonePair.first.begin(), bonePair.first.end()) + " " + val, LogLevel::Info);
        // setBoneSpaceLocalRotator(anim.component, fname_from_string(bonePair.first), localRotator, 0);
        // You need to implement setBoneSpaceLocalRotator in C++
    }
}

// Lerp animation logic (stub, as RLerp and transform math are engine-specific)
inline void lerpAnimation(const std::string& animID, const std::string& animName, float alpha) {
    // Implement RLerp logic as needed
}

// Pose logic
inline void pose(const std::string& animID, const std::string& poseID) {
    animation_print("Called pose " + poseID + " for animationID " + animID, LogLevel::Debug);
    auto it = animations.find(animID);
    if (it == animations.end() || !it->second.definitions) return;
    auto poseIt = it->second.definitions->poses.find(poseID);
    if (poseIt == it->second.definitions->poses.end()) return;
    animation_print("Found pose " + poseID, LogLevel::Debug);
    int i = 0;
    for (const auto& positions : poseIt->second) {
        animation_print("Animating pose index " + std::to_string(i++) + " " + animID + " " + positions.first + " " + positions.second, LogLevel::Info);
        animate(animID, positions.first, positions.second);
    }
}

// Add animation instance
inline void add(const std::string& animID, API::UObject* skeletalMeshComponent, std::shared_ptr<AnimationDefinition> animationDefinitions) {
    animations[animID] = AnimationInstance{ skeletalMeshComponent, animationDefinitions };
}

// Update animation state
inline void updateAnimation(const std::string& animID, const std::string& animName, bool isPressed, /*LerpParams*/ void* lerpParam = nullptr) {
    auto& state = animStates[animID][animName];
    if (isPressed) {
        if (!state) {
            // If lerpParam, call lerp, else animate
            animate(animID, animName, "on");
        }
        state = true;
    }
    else {
        if (state) {
            // If lerpParam, call lerp, else animate
            animate(animID, animName, "off");
        }
        state = false;
    }
}

inline void resetAnimation(const std::string& animID, const std::string& animName, bool isPressed) {
    animStates[animID][animName] = isPressed;
}

// Visualization helpers (stub, as these require engine-specific mesh/component logic)
inline void createSkeletalVisualization(API::UObject* skeletalMeshComponent, float scale = 0.003f) {
    // Implement visualization logic as needed
}

inline void updateSkeletalVisualization(API::UObject* skeletalMeshComponent) {
    // Implement visualization update logic as needed
}

inline void setSkeletalVisualizationBoneScale(API::UObject* skeletalMeshComponent, int index, float scale) {
    // Implement visualization scale logic as needed
}


// -------------------- DEBUG --------------------

class Debug {
public:
    static void dump(API::UObject* object, bool recursive = false, std::set<std::wstring>* ignoreRecursionList = nullptr, int level = 0) {
        if (!object) {
            API::get()->log_info("Invalid parameters passed to dumpObject");
            return;
        }
        std::wstring indent(level * 2, L' ');
        API::get()->log_info(L"%sObject: %ls", indent.c_str(), object->get_full_name().c_str());

        // Dump properties
        auto klass = object->get_class();
        while (klass) {
            auto property = klass->get_child_properties();
            while (property) {
                std::wstring propName = property->get_fname()->to_string();
                // Print property name and value (pseudo-code, adapt to your API)
                // API::get()->log_info(L"%s  %ls = ...", indent.c_str(), propName.c_str());
                property = property->get_next();
            }
            klass = klass->get_super_struct();
        }
        // Recursion and ignore list logic can be added as needed
    }
};

} // namespace uevr_utils
