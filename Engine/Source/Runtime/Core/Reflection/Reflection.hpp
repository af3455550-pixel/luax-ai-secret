#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <typeindex>
#include <iostream>

namespace Apex::Reflection {

    enum class PropertyType {
        Int32,
        Float,
        Double,
        Bool,
        String,
        Vec2,
        Vec3,
        Vec4,
        Quat,
        CustomObject
    };

    struct Property {
        std::string name;
        PropertyType type;
        size_t offset;
        size_t size;

        std::function<std::string(const void*)> toString;
        std::function<void(void*, const std::string&)> fromString;
    };

    class ClassType {
    public:
        ClassType(std::string name, size_t size) : m_name(std::move(name)), m_size(size) {}

        const std::string& GetName() const { return m_name; }
        size_t GetSize() const { return m_size; }

        ClassType& AddProperty(Property prop) {
            m_properties.push_back(std::move(prop));
            return *this;
        }

        const std::vector<Property>& GetProperties() const { return m_properties; }

        const Property* FindProperty(const std::string& propName) const {
            for (const auto& prop : m_properties) {
                if (prop.name == propName) return &prop;
            }
            return nullptr;
        }

    private:
        std::string m_name;
        size_t m_size;
        std::vector<Property> m_properties;
    };

    class Registry {
    public:
        static Registry& Get() {
            static Registry instance;
            return instance;
        }

        template <typename T>
        ClassType& RegisterClass(const std::string& name) {
            std::type_index id = std::type_index(typeid(T));
            auto type = std::make_unique<ClassType>(name, sizeof(T));
            ClassType* raw = type.get();
            m_types[id] = std::move(type);
            m_nameToType[name] = raw;
            return *raw;
        }

        template <typename T>
        const ClassType* GetClass() const {
            auto it = m_types.find(std::type_index(typeid(T)));
            if (it != m_types.end()) return it->second.get();
            return nullptr;
        }

        const ClassType* GetClassByName(const std::string& name) const {
            auto it = m_nameToType.find(name);
            if (it != m_nameToType.end()) return it->second;
            return nullptr;
        }

        const std::unordered_map<std::string, ClassType*>& GetAllClasses() const {
            return m_nameToType;
        }

    private:
        std::unordered_map<std::type_index, std::unique_ptr<ClassType>> m_types;
        std::unordered_map<std::string, ClassType*> m_nameToType;
    };

} // namespace Apex::Reflection

#define APEX_PROPERTY(Class, Member, TypeEnum) \
    .AddProperty({ \
        #Member, \
        Apex::Reflection::PropertyType::TypeEnum, \
        offsetof(Class, Member), \
        sizeof(Class::Member), \
        [](const void* obj) -> std::string { \
            const auto* typed = static_cast<const Class*>(obj); \
            std::ostringstream ss; ss << typed->Member; return ss.str(); \
        }, \
        [](void* obj, const std::string& val) { \
            auto* typed = static_cast<Class*>(obj); \
            std::istringstream ss(val); ss >> typed->Member; \
        } \
    })
