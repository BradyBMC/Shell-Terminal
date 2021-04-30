// $Id: file_sys.cpp,v 1.10 2021-04-10 14:23:40-07 - - $

#include <cassert>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace std;

#include "debug.h"
#include "file_sys.h"

size_t inode::next_inode_nr {1};

ostream& operator<< (ostream& out, file_type type) {
   switch (type) {
      case file_type::PLAIN_TYPE: out << "PLAIN_TYPE"; break;
      case file_type::DIRECTORY_TYPE: out << "DIRECTORY_TYPE"; break;
      default: assert (false);
   };
   return out;
}

inode_state::inode_state() {
   inode rut = inode(file_type::DIRECTORY_TYPE);
   root = make_shared<inode>(rut);
   root->name = "/";
   root->contents->setup_dir(cwd, root);
   cwd = root;
   DEBUGF ('i', "root = " << root->name << ", cwd = " << cwd
         << ", prompt = \"" << prompt() << "\"");

}

void inode_state::make_directory(const wordvec& dirname) {
  inode_ptr path = directory_search(dirname, cwd);
  if(path == nullptr) {
    cout << "ILLEGAL DIRECTORY PATH" << endl;
    return;
  }
  map<string, inode_wk_ptr> parent = path->get_higher();
  map<string, inode_ptr> children = path->get_lower();
  cout << children.size() << endl;
  inode_ptr n_dir = path->contents->mkdir(dirname[dirname.size() - 1]);
  cout << n_dir->name << endl;
  n_dir->contents->setup_dir(n_dir, cwd);
  children.insert(pair<string, inode_ptr>(n_dir->name, n_dir));
  cwd->set_lower(children);
  cout << "Task::Completed" << endl;
  cout << "size :: " << cwd->get_lower().size() << endl;
  for(auto const &pair:children) {
    cout << pair.first << " " << pair.second << endl;
  }
}

inode_ptr inode_state::directory_search(const wordvec& input,
                                                 inode_ptr curr){
  //Does not catch if directory already has name
  for(int i = 0;i < static_cast<int>(input.size()) - 1;i++) {
    cout << "runs " << endl;
    map<string, inode_wk_ptr> parent = curr->get_higher();
    map<string,inode_ptr> child = curr->get_lower();
    string name = input[i];
    if(".." == name || "." == name) {
      curr = parent[name].lock();
    } else {
      map<string,inode_ptr>::iterator it;
      it = child.find(name);
      //Illegal path
      if(it == child.end()) {
        return nullptr;
      }
      curr = child[name];
    }
  }
  return curr;
}

const string& inode_state::prompt() const { return prompt_; }

ostream& operator<< (ostream& out, const inode_state& state) {
   out << "inode_state: root = " << state.root
       << ", cwd = " << state.cwd;
   return out;
}

inode::inode(file_type type): inode_nr (next_inode_nr++) {
   switch (type) {
      case file_type::PLAIN_TYPE:
           contents = make_shared<plain_file>();
           break;
      case file_type::DIRECTORY_TYPE:
           contents = make_shared<directory>();
           break;
      default: assert (false);
   }
   DEBUGF ('i', "inode " << inode_nr << ", type = " << type);
}

size_t inode::get_inode_nr() const {
   DEBUGF ('i', "inode = " << inode_nr);
   return inode_nr;
}

map<string, inode_wk_ptr> inode::get_higher() {
  return contents->get_parent();
}

map<string, inode_ptr> inode::get_lower() {
  return contents->get_children();
}

void inode::set_lower(const map<string, inode_ptr>& child) {
  contents->set_children(child);
}

void inode::set_name(string input) {
  name = input;
}


file_error::file_error (const string& what):
            runtime_error (what) {
}

const wordvec& base_file::readfile() const {
   throw file_error ("is a " + error_file_type());
}

void base_file::writefile (const wordvec&) {
   throw file_error ("is a " + error_file_type());
}

void base_file::remove (const string&) {
   throw file_error ("is a " + error_file_type());
}

inode_ptr base_file::mkdir (const string&) {
   throw file_error ("is a " + error_file_type());
}

inode_ptr base_file::mkfile (const string&) {
   throw file_error ("is a " + error_file_type());
}

void base_file::setup_dir (const inode_ptr&, inode_ptr&) {
   throw file_error ("is a " + error_file_type());
}

map<string,inode_ptr> base_file::get_children() {
  throw file_error ("is a " + error_file_type());
}

map<string,inode_wk_ptr> base_file::get_parent() {
  throw file_error ("is a "+ error_file_type());
}

void base_file::set_children(const map<string,inode_ptr>&) {
  throw file_error("is a " + error_file_type());
}


size_t plain_file::size() const {
   size_t size {0};
   DEBUGF ('i', "size = " << size);
   return size;
}

const wordvec& plain_file::readfile() const {
   DEBUGF ('i', data);
   return data;
}

void plain_file::writefile (const wordvec& words) {
   DEBUGF ('i', words);
}

size_t directory::size() const {
   size_t size {0};
   DEBUGF ('i', "size = " << size);
   return size;
}

void directory::remove (const string& filename) {
   DEBUGF ('i', filename);
} 

inode_ptr directory::mkdir (const string& dirname) {
   inode_ptr n_dir = make_shared<inode>(file_type::DIRECTORY_TYPE);
   n_dir->set_name(dirname);
   DEBUGF ('i', dirname);
   return n_dir;
}

inode_ptr directory::mkfile (const string& filename) {
   DEBUGF ('i', filename);
   return nullptr;
}

void directory::setup_dir (const inode_ptr& cwd, inode_ptr& parent ) {
  inode_wk_ptr current_dir = cwd;
  inode_wk_ptr parent_dir = parent;
  wk_dirents.insert(pair<string, inode_wk_ptr>(".", current_dir));
  wk_dirents.insert(pair<string, inode_wk_ptr>("..", parent_dir));
}

map<string,inode_ptr> directory::get_children() {
  return dirents;
}

map<string,inode_wk_ptr> directory::get_parent() {
  return wk_dirents;
}

void directory::set_children(const map<string,inode_ptr>& child) {
  dirents = child;
}
