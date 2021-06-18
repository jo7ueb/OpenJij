//    Copyright 2021 Jij Inc.

//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at

//        http://www.apache.org/licenses/LICENSE-2.0

//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

//! @file polynomial.hpp
//! @brief Graph class to represent polynomial unconstrained binary model or Ising model with polynomial interactions.
//! @date 2021-03-11
//! @copyright Copyright (c) Jij Inc. 2021

#ifndef polynomial_hpp
#define polynomial_hpp

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <unordered_map>
#include <graph/json/parse.hpp>
#include <graph/graph.hpp>
#include "cimod/src/binary_polynomial_model.hpp"

namespace openjij {
namespace graph {

//! @brief Polynomial graph class, which can treat many-body interactions.
//! The Hamiltonian is like
//! \f[
//! H=\sum_{i \neq j} Q_{ij} x_i x_j +  \sum_{i \neq j \neq k} Q_{ijk} x_i x_j x_k + \ldots
//! \f]
//! Note here that \f$ x_i \in \{0, 1\} \f$ or \f$ x_i \in \{-1, +1\} \f$.
//! @tparam FloatType floating-point type
template<typename FloatType>
class Polynomial: public Graph {
   static_assert(std::is_floating_point<FloatType>::value, "FloatType must be floating-point type.");
   
public:
   
   //! @brief Floating-point type
   using value_type = FloatType;
   
   //! @brief Constructor of Polynomial class to initialize variables and vartype.
   //! @param num_variables std::size_t
   //! @param vartype cimod::Vartype
   Polynomial(const std::size_t num_variables, const cimod::Vartype &vartype): Graph(num_variables), vartype_(vartype) {}

   //! @brief Constructor of Polynomial class to initialize variables and vartype.
   //! @param num_variables std::size_t
   //! @param vartype SPIN or BINARY (std::string)
   Polynomial(const std::size_t num_variables, const std::string vartype): Graph(num_variables), vartype_(ConvertVartype(vartype)) {}
    
   //! @brief Constructor of Polynomial class to initialize num_variables, vartype, and interactions from json by using a delegating constructor.
   //! @param j JSON object
   explicit Polynomial(const nlohmann::json &j): Graph(j.at("variables").size()) {
      const auto &v_k_v = json_parse_polynomial<FloatType>(j);
      const auto &num_variable    = std::get<0>(v_k_v);
      const auto &poly_key_list   = std::get<1>(v_k_v);
      const auto &poly_value_list = std::get<2>(v_k_v);
      
      if (poly_key_list.size() != poly_value_list.size()) {
         throw std::runtime_error("The sizes of key_list and value_list must match each other");
      }
      
      vartype_ = ConvertVartype(j.at("vartype"));
      
      int64_t num_interactions = static_cast<int64_t>(poly_key_list.size());
            
      poly_key_list_.resize(num_interactions);
      poly_value_list_.resize(num_interactions);
      
#pragma omp parallel for
      for (int64_t i = 0; i < num_interactions; ++i) {
         poly_key_list_[i]   = poly_key_list[i];
         poly_value_list_[i] = poly_value_list[i];
      }
      
      for (std::size_t i = 0; i < poly_key_list.size(); ++i) {
         poly_key_inv_[poly_key_list_[i]] = i;
      }
      
   }
   
   //! @brief Access the interaction corresponding to the input argument "std::vector<Index>& index" (lvalue references) to set an interaction.
   //! @param key std::vector<Index>&
   //! @return The interaction corresponding to "std::vector<Index>& index", i.e., J[index]
   FloatType &J(std::vector<Index> &key) {
      std::sort(key.begin(), key.end());
      CheckKeyValid(key);
      if (poly_key_inv_.count(key) == 0) {
         poly_key_inv_[key] = poly_value_list_.size();
         poly_key_list_.push_back(key);
         poly_value_list_.push_back(0.0);
      }
      return poly_value_list_[poly_key_inv_.at(key)];
   }
   
   //! @brief Access the interaction corresponding to the input argument "const std::vector<Index>& index" (lvalue references) to set an interaction.
   //! @param index const std::vector<Index>&
   //! @return The interaction corresponding to "const std::vector<Index>& index", i.e., J[index]
   FloatType &J(const std::vector<Index> &key) {
      std::vector<Index> copied_key = key;
      return J(copied_key);
   }
   
   //! @brief Return the interaction corresponding to the input argument "std::vector<Index> &index" (lvalue references).
   //! @param index std::vector<Index>&
   //! @return The interaction corresponding to "std::vector<Index>& index", i.e., J.at(index)
   FloatType J(std::vector<Index> &key) const {
      std::sort(key.begin(), key.end());
      CheckKeyValid(key);
      if (poly_key_inv_.count(key) == 0) {
         return 0.0;
      }
      else {
         return poly_value_list_[poly_key_inv_.at(key)];
      }
   }
   
   //! @brief Return the interaction corresponding to the input argument "const std::vector<Index> &index".
   //! @param index const std::vector<Index>&
   //! @return The interaction corresponding to "const std::vector<Index>& index", i.e., J.at(index)
   FloatType J(const std::vector<Index> &key) const {
      std::vector<Index> copied_key = key;
      return J(copied_key);
   }
   
   //! @brief Access the interaction corresponding to the input argument "args" (parameter pack) to set an interaction.
   //! @param args parameter pack
   //! @return The interaction corresponding to "args", i.e., J[args]
   template<typename... Args>
   FloatType &J(Args... args) {
      std::vector<Index> copied_key{(Index)args...};
      return J(copied_key);
   }
   
   //! @brief Return the interaction corresponding to the input argument "args" (parameter pack).
   //! @param args parameter pack
   //! @return The interaction corresponding to "args", i.e., J[args]
   template<typename... Args>
   FloatType J(Args... args) const {
      std::vector<Index> copied_key{(Index)args...};
      return J(copied_key);
   }
   
   //! @brief Return the polynomial interactions.
   //! @return The interactions
   cimod::Polynomial<Index, FloatType> get_polynomial() const {
      cimod::Polynomial<Index, FloatType> poly_map;
      for (std::size_t i = 0; i < poly_key_list_.size(); ++i) {
         poly_map[poly_key_list_[i]] = poly_value_list_[i];
      }
      return poly_map;
   }
   
   //! @brief Get the PolynomialKeyList object.
   //! @return PolynomialKeyList object as std::vector<std::vector>>.
   const cimod::PolynomialKeyList<Index> &get_keys() const {
      return poly_key_list_;
   }
   
   //! @brief Get the PolynomialValueList object.
   //! @return PolynomialValueList object as std::vector.
   const cimod::PolynomialValueList<FloatType> &get_values() const {
      return poly_value_list_;
   }
   
   //! @brief Return the vartype.
   //! @return The vartype
   cimod::Vartype get_vartype() const {
      return vartype_;
   }
   
   //! @brief Set vartype.
   //! @param vartype cimod::Vartype
   void set_vartype(const cimod::Vartype vartype) {
      vartype_ = vartype;
   }
   
   //! @brief Set vartype.
   //! @param vartype std::string
   void set_vartype(const std::string vartype) {
      vartype_ = ConvertVartype(vartype);
   }
   
   //! @brief Return the number of interactions
   //! @return The number of interactions
   std::size_t get_num_interactions() const {
      return poly_key_list_.size();
   }
   
   //! @brief Return the total energy corresponding to the input variables, Spins or Binaries.
   //! @param spins const Spins& or const Binaries& (both are the same type)
   //! @param omp_flag if true OpenMP is enabled.
   //! @return The total energy
   FloatType energy(const Spins &spins, const bool omp_flag = true) const {
      if(spins.size() != Graph::size()){
         throw std::out_of_range("The size of spins/binaries does not equal to the size of polynomial graph");
      }
      
      FloatType energy = 0.0;
      
      int64_t num_interactions = static_cast<int64_t>(poly_key_list_.size());
      
      if (omp_flag) {
#pragma omp parallel for reduction (+: energy)
         for (int64_t i = 0; i < num_interactions; ++i) {
            Spin spin_multiple = 1;
            for (const auto &index: poly_key_list_[i]) {
               spin_multiple *= spins[index];
               if (spin_multiple == 0.0) {
                  break;
               }
            }
            energy += spin_multiple*poly_value_list_[i];
         }
      }
      else {
         for (std::size_t i = 0; i < num_interactions; ++i) {
            Spin spin_multiple = 1;
            for (const auto &index: poly_key_list_[i]) {
               spin_multiple *= spins[index];
               if (spin_multiple == 0.0) {
                  break;
               }
            }
            energy += spin_multiple*poly_value_list_[i];
         }
      }
      return energy;
   }

   //! @deprecated
   //! @brief Return the total energy corresponding to the input variables, Spins or Binaries.
   //! @param spins const Spins& or const Binaries& (both are the same type)
   //! @param omp_flag if true OpenMP is enabled.
   //! @return The total energy
   FloatType calc_energy(const Spins &spins, const bool omp_flag = true) const {
      return energy(spins, omp_flag);
   }
   
private:
   //! @brief The list of the indices of the polynomial interactions (namely, the list of keys of the polynomial interactions as std::unordered_map) as std::vector<std::vector>>.
   cimod::PolynomialKeyList<Index> poly_key_list_;
   
   //! @brief The list of the values of the polynomial interactions (namely, the list of values of the polynomial interactions as std::unordered_map) as std::vector.
   cimod::PolynomialValueList<FloatType> poly_value_list_;
   
   //! @brief The inverse key list, which indicates the index of the poly_key_list_ and poly_value_list_
   std::unordered_map<std::vector<Index>, std::size_t, cimod::vector_hash> poly_key_inv_;
   
   //! @brief The model's type. SPIN or BINARY
   cimod::Vartype vartype_ = cimod::Vartype::NONE;
   
   //! @brief Convert variable type from string
   //! @param str const std::string
   //! @return cimod::Vartype
   cimod::Vartype ConvertVartype(const std::string str) const {
      if (str == "BINARY") {
         return cimod::Vartype::BINARY;
      }
      else if (str == "SPIN") {
         return cimod::Vartype::SPIN;
      }
      else {
         throw std::runtime_error("Unknown vartype detected");
      }
   }
   
   //! @brief Check if the input keys are valid
   void CheckKeyValid(const std::vector<Index> &key) const {
      if (key.size() > Graph::size()) {
         std::stringstream ss;
         ss << "Too small system size. ";
         ss << "The degree of the input polynomial interaction is " << key.size();
         ss << ". But the system size is " << Graph::size();
         throw std::runtime_error(ss.str());
      }
      if (0 < key.size()) {
         //key is assumed to be sorted
         for (std::size_t i = 0; i < key.size() - 1; ++i) {
            if (key[i] == key[i + 1]) {
               throw std::runtime_error("No self-loops allowed");
            }
            if (key[i] >= Graph::size()) {
               std::stringstream ss;
               ss << "Too small system size. ";
               ss << "The index of a interaction: " << key[i] << " is out of range";
               throw std::runtime_error(ss.str());
            }
         }
         if (key.back() >= Graph::size()) {
            std::stringstream ss;
            ss << "Too small system size. ";
            ss << "The index of a interaction: " << key.back() << " is out of range";
            throw std::runtime_error(ss.str());
         }
      }
   }
   
};
 
} //graph
} //openjij


#endif /* polynomial_hpp */
