/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file shaderModuleSpirV.h
 * @author rdb
 * @date 2019-07-15
 */

#ifndef SHADERMODULESPIRV_H
#define SHADERMODULESPIRV_H

#include "shader.h"
#include "spirv.hpp"

class ShaderType;

/**
 * ShaderModule that contains compiled SPIR-V bytecode.  This class can extract
 * the parameter definitions from the bytecode, assign appropriate locations,
 * link the module to a previous stage, and strip debug information as needed.
 */
class EXPCL_PANDA_SHADERPIPELINE ShaderModuleSpirV final : public ShaderModule {
public:
  ShaderModuleSpirV(Stage stage, std::vector<uint32_t> words);
  virtual ~ShaderModuleSpirV();

  virtual PT(CopyOnWriteObject) make_cow_copy() override;

  INLINE const uint32_t *get_data() const;
  INLINE size_t get_data_size() const;

  virtual bool link_inputs(const ShaderModule *previous) override;
  virtual void remap_parameter_locations(pmap<int, int> &remap) override;

  virtual std::string get_ir() const override;

  class InstructionStream;

  struct Instruction {
    const spv::Op opcode;
    const uint32_t nargs;
    uint32_t *args;
  };

  class InstructionIterator {
  public:
    constexpr InstructionIterator() = default;

    INLINE Instruction operator *();
    INLINE InstructionIterator &operator ++();
    INLINE bool operator ==(const InstructionIterator &other) const;
    INLINE bool operator !=(const InstructionIterator &other) const;

  private:
    INLINE InstructionIterator(uint32_t *words);

    uint32_t *_words = nullptr;

    friend class InstructionStream;
  };

  /**
   * A container that allows conveniently iterating over the instructions.
   */
  class InstructionStream {
  public:
    typedef InstructionIterator iterator;

    InstructionStream() = default;
    INLINE InstructionStream(const uint32_t *words, size_t num_words);
    INLINE InstructionStream(std::vector<uint32_t> words);

    bool validate_header() const;

    INLINE operator std::vector<uint32_t> & ();

    InstructionStream strip() const;

    INLINE iterator begin();
    INLINE iterator begin_annotations();
    INLINE iterator end();
    INLINE iterator insert(iterator &it, spv::Op opcode, std::initializer_list<uint32_t > args);
    INLINE iterator insert(iterator &it, spv::Op opcode, const uint32_t *args, uint16_t nargs);
    INLINE iterator erase(iterator &it);
    INLINE iterator erase_arg(iterator &it, uint16_t arg);

    INLINE const uint32_t *get_data() const;
    INLINE size_t get_data_size() const;

    INLINE uint32_t get_id_bound() const;
    INLINE uint32_t allocate_id();

  private:
    // We're not using a pvector since glslang/spirv-opt are working with
    // std::vector<uint32_t> and so we can avoid some unnecessary copies.
    std::vector<uint32_t> _words;
  };

  InstructionStream _instructions;

protected:
  enum DefinitionType {
    DT_none,
    DT_type,
    DT_type_pointer,
    DT_variable,
    DT_constant,
    DT_ext_inst,
  };

  /**
   * Temporary structure to hold a single definition, which could be a variable,
   * type or type pointer in the SPIR-V file.
   */
  struct Definition {
    DefinitionType _dtype = DT_none;
    std::string _name;
    const ShaderType *_type = nullptr;
    int _location = -1;
    spv::BuiltIn _builtin = spv::BuiltInMax;
    uint32_t _constant = 0;
    vector_string _member_names;
    bool _used = false;

    // Only defined for DT_variable.
    spv::StorageClass _storage_class;

    void set_name(const char *name);
    void set_member_name(uint32_t i, const char *name);

    void set_type(const ShaderType *type);
    void set_type_pointer(spv::StorageClass storage_class, const ShaderType *type);
    void set_variable(const ShaderType *type, spv::StorageClass storage_class);
    void set_constant(const ShaderType *type, const uint32_t *words, uint32_t nwords);
    void set_ext_inst(const char *name);

    void mark_used();

    void clear();
  };
  typedef pvector<Definition> Definitions;

private:
  bool parse(Definitions &defs);
  bool parse_instruction(Definitions &defs, spv::Op opcode,
                         const uint32_t *args, size_t nargs);

  void assign_locations(Definitions &defs);
  void remap_locations(spv::StorageClass storage_class, const pmap<int, int> &locations);

  void flatten_struct(Definitions &defs, uint32_t type_id);
  void strip();

  int _index;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    ShaderModule::init_type();
    register_type(_type_handle, "ShaderModuleSpirV",
                  ShaderModule::get_class_type());
  }
  virtual TypeHandle get_type() const override {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() override {
    init_type();
    return get_class_type();
  }

private:
  static TypeHandle _type_handle;
};

#include "shaderModuleSpirV.I"

#endif