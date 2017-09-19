//===--- NamespaceNamingCheck.cpp - clang-tidy-----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "NamespaceNamingCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <regex>
#include <string>
#include <ctype.h>
 
using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace aliceO2 {
 
const std::string VALID_NAME_REGEX = "[a-z][a-z_0-9]+";
const std::string VALID_PATH_REGEX = "(.*/O2/.*)|(.*/test/.*)";

bool isOutsideOfTargetScope(std::string filename)
{
  return !std::regex_match(filename, std::regex(VALID_PATH_REGEX));
}

void NamespaceNamingCheck::registerMatchers(MatchFinder *Finder) {
  const auto validNameMatch = matchesName( std::string("::") + VALID_NAME_REGEX + "$" );
  
  // matches namespace declarations that have invalid name
  Finder->addMatcher(namespaceDecl(allOf(
    unless(validNameMatch),
    unless(isAnonymous())
    )).bind("namespace-decl"), this);
  // matches usage of namespace
  Finder->addMatcher(nestedNameSpecifierLoc(loc(nestedNameSpecifier(specifiesNamespace(unless(validNameMatch)
    )))).bind("namespace-usage"), this );
  // matches "using namespace" declarations
  Finder->addMatcher(usingDirectiveDecl(unless(isImplicit()
    )).bind("using-namespace"), this);
}

void NamespaceNamingCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedNamespaceDecl = Result.Nodes.getNodeAs<NamespaceDecl>("namespace-decl");
  if( MatchedNamespaceDecl )
  {
    if( isOutsideOfTargetScope( Result.SourceManager->getFilename(MatchedNamespaceDecl->getLocation()).str() ) )
    {
      return;
    }
  
    std::string newName(MatchedNamespaceDecl->getDeclName().getAsString());
    
    fixNamespaceName(newName);
    
    diag(MatchedNamespaceDecl->getLocation(), "namespace %0 does not follow the underscore convention")
        << MatchedNamespaceDecl
        << FixItHint::CreateReplacement(MatchedNamespaceDecl->getLocation(), newName);
  }
  
  const auto *MatchedNamespaceLoc = Result.Nodes.getNodeAs<NestedNameSpecifierLoc>("namespace-usage");
  if( MatchedNamespaceLoc )
  {
    const auto *AsNamespace = MatchedNamespaceLoc->getNestedNameSpecifier()->getAsNamespace();
    if( isOutsideOfTargetScope( Result.SourceManager->getFilename(AsNamespace->getLocation()).str() ) )
    {
      return;
    }
    std::string newName(AsNamespace->getDeclName().getAsString());
    
    fixNamespaceName(newName);
    
    diag(MatchedNamespaceLoc->getLocalBeginLoc(), "namespace %0 does not follow the underscore convention")
        << AsNamespace
        << FixItHint::CreateReplacement(MatchedNamespaceLoc->getLocalBeginLoc(), newName);
  }

  const auto *MatchedUsingNamespace = Result.Nodes.getNodeAs<UsingDirectiveDecl>("using-namespace");
  if( MatchedUsingNamespace )
  {
    if( isOutsideOfTargetScope( Result.SourceManager->getFilename(MatchedUsingNamespace->getNominatedNamespace()->getLocation()).str() ) )
    {
      return;
    }
    
    std::string newName(MatchedUsingNamespace->getNominatedNamespace()->getDeclName().getAsString());
    
    if( std::regex_match(newName, std::regex(VALID_NAME_REGEX)) )
    {
      return;
    }
    
    fixNamespaceName(newName);
    
    diag(MatchedUsingNamespace->getLocation(), "namespace %0 does not follow the underscore convention")
        << MatchedUsingNamespace->getNominatedNamespace()
        << FixItHint::CreateReplacement(MatchedUsingNamespace->getLocation(), newName);
  }
}

void NamespaceNamingCheck::fixNamespaceName(std::string &name)
{
  for(int i=name.size()-1; i>=0; i--) {
    if(isupper(name[i])) {
      if(i != 0 && islower(name[i-1])) {
        name.insert(i, "_");
        i++;
      }
      name[i] = tolower(name[i]);
    }
  }
}

} // namespace aliceO2
} // namespace tidy
} // namespace clang