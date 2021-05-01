// $Id: file_sys.cpp,v 1.10 2021-04-10 14:23:40-07 - - $

#include <cassert>
#include <iostream>
#include <iterator>
#include <stack>
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
   root->contents->setup_dir(root, root);
   cwd = root;
   DEBUGF ('i', "root = " << root->name << ", cwd = " << cwd
         << ", prompt = \"" << prompt() << "\"");

}

void inode_state::make_directory(const wordvec& dirname) {
  if(dirname.size() == 0) {
    cout << "No input" << endl;
    return;
  }
  inode_ptr path = directory_search(dirname, cwd, true);
  if(path == nullptr) {
    cout << "ILLEGAL DIRECTORY PATH" << endl;
    return;
  }
  map<string, inode_wk_ptr> parent = path->get_higher();
  map<string, inode_ptr> children = path->get_lower();
  inode_ptr n_dir = path->contents->mkdir(dirname[dirname.size() - 1] + "/");
  n_dir->contents->setup_dir(n_dir, path);
  children.insert(pair<string, inode_ptr>(n_dir->name, n_dir));
  path->set_lower(children);
  /*
  cout << "Task::Completed" << endl;
  cout << "size :: " << cwd->get_lower().size() << endl;
  for(auto const &pair:children) {
    cout << pair.first << " " << pair.second << endl;
  }
  */
}

void inode_state::change_directory(const wordvec& dirname) {
  /*
  inode_ptr curr = cwd;
  for(int i = 0;i < static_cast<int>(dirname.size());i++) {
    map<string, inode_wk_ptr> parent = curr->get_higher();
    map<string,inode_ptr> child = curr->get_lower();
    string name = dirname[i];
    if(".." == name || "." == name) {
      curr = parent[name].lock();
    } else {
      map<string,inode_ptr>::iterator it;
      it = child.find(name);
      //Illegal path
      if(it == child.end()) {
        cout << "ILLEGAL DIRECTORY PATH" << endl;
        return;
      }
      curr = child[name];
    }
  }
  cwd = curr;
  */
  if(dirname.size() == 0) {
    cwd = root;
  } else {
    cwd = directory_search(dirname, cwd, false);
  }
}

void inode_state::make_file(const wordvec& words) {
  wordvec path = split(words.at(1), "/"); // get path
  DEBUGF('f', "path: " << path);

  wordvec n_data; // data to write to new file with "" if none
  n_data.push_back("");
  DEBUGF('f', "data: " << n_data);
  if (words.size() > 2) {
    for (size_t i = 2; i != words.size(); ++i) {
      n_data.push_back(words.at(i));
    }
  }
  
  inode_ptr temp = directory_search(path, cwd);
  DEBUGF('f', "temp made: " << temp);
  if (temp == nullptr)
  {
    cout << "ILLEGAL DIRECTORY PATH" << endl;
    return;
  }
  inode_ptr n_file = temp->contents->mkfile(path[path.size() - 1]);
  DEBUGF('f', "n_file: " <<  n_file);
  n_file->contents->writefile(n_data);
  
  map<string, inode_ptr> children = temp->get_lower();
  children.insert(pair<string, inode_ptr>(n_file->name, n_file));
  temp->set_lower(children);
  
  /*
  for(auto const &pair:children) {
    cout << pair.first << " " << pair.second << endl;
  }
  */
}

inode_ptr inode_state::directory_search(const wordvec& input,
                                                 inode_ptr curr, bool make){
  //Does not catch if directory already has name
  int x = make ? 1 : 0;
  for(int i = 0;i < static_cast<int>(input.size()) - x;i++) {
    map<string, inode_wk_ptr> parent = curr->get_higher();
    map<string,inode_ptr> child = curr->get_lower();
    string name = input[i] + "/";
    if("../" == name || "./" == name) {
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

void inode_state::list(const wordvec& path) {
  /*
  cout << "~~~~~" << endl;
  cout << "LIST : TASK" << endl;
  */
  inode_ptr curr = directory_search(path, cwd, false);
  if(curr == nullptr) {
    cout << "ILLEGAL DIRECTORY PATH" << endl;
    return;
  }
  map<string, inode_wk_ptr> parent = curr->get_higher();
  map<string, inode_ptr> children = curr->get_lower();
  for(auto const &pair:parent) {
    string name = pair.first;
    cout << name << " " << parent[name].lock()->get_inode_nr() << endl;
  }
  for(auto const &pair:children) {
    string name = pair.first;
    cout << name << " " << children[name]->get_inode_nr() << endl;
  }
}

void inode_state::print_working_directory() {
  stack<string> path;
  path.push(cwd->name);
  inode_ptr curr = cwd;
  while(curr != root) {
    map<string, inode_wk_ptr> parent = curr->get_higher();
    curr = parent["../"].lock();
    path.push(curr->name);
  }
  while(!path.empty()) {
    cout << path.top();
    path.pop();
  }
  cout << endl;
  //cout << path << endl;
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
   size = data.size();
   DEBUGF ('i', "size = " << size);
   return size;
}

inode_ptr plain_file::mkfile(const string& filename){
  inode_ptr file_ptr = make_shared<inode>(file_type::PLAIN_TYPE);
  file_ptr->set_name(filename);
  DEBUGF('i', filename);
  return file_ptr;
}

const wordvec& plain_file::readfile() const {
   DEBUGF ('i', data);
   return data;
}

void plain_file::writefile (const wordvec& words) {
   this->data = std::move(words);
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
   /*
   DEBUGF ('i', filename);
   return nullptr;
   */
  inode_ptr file_ptr = make_shared<inode>(file_type::PLAIN_TYPE);
  file_ptr->set_name(filename);
  DEBUGF('i', filename);
  return file_ptr;
}

void directory::setup_dir (const inode_ptr& cwd, inode_ptr& parent ) {
  inode_wk_ptr current_dir = cwd;
  inode_wk_ptr parent_dir = parent;
  wk_dirents.insert(pair<string, inode_wk_ptr>("./", current_dir));
  wk_dirents.insert(pair<string, inode_wk_ptr>("../", parent_dir));
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
