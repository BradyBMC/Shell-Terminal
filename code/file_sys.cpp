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
   root->size+=2;
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
  inode_ptr n_dir=path->contents->mkdir(dirname[dirname.size()-1]+"/");
  increment_recursive(path);
  n_dir->contents->setup_dir(n_dir, path);
  n_dir->size+=2;
  children.insert(pair<string, inode_ptr>(n_dir->name, n_dir));
  path->set_lower(children);
}

void inode_state::change_directory(const wordvec& dirname) {
  if(dirname.size() == 0) {
    cwd = root;
  } else {
    inode_ptr temp = directory_search(dirname, cwd, false);
    if(temp != NULL) {
      cwd = temp;
    }
  }
}

void inode_state::make_file(const wordvec& words) {
  wordvec path = split(words.at(1), "/"); // get path
  DEBUGF('f', "path: " << path);

  wordvec n_data; // data to write to new file with "" if none
  DEBUGF('f', "data: " << n_data);
  if (words.size() > 2) {
    for (size_t i = 2; i != words.size(); ++i) {
      n_data.push_back(words.at(i));
    }
  } else {
    n_data.push_back("");
  }
  
  inode_ptr temp = directory_search(path, cwd, true);
  /*
  if (temp == nullptr) {
    wordvec n_path = path;
    n_path.pop_back();
    temp = directory_search(n_path, cwd, false);
    map<string, inode_ptr> child = temp->get_lower();
    map<string,inode_ptr>::iterator it;
    it = child.find(path.at(path.size()-1));
    //Illegal path
    if(it == child.end()) {
      cout << "no file" << endl;
      return;
    }
    temp = child[path.at(path.size()-1)];
    temp->contents->writefile(n_data);
  }
  */
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
  
}

void inode_state::print_file(const wordvec& words) {
  wordvec path = split(words.at(1), "/");
  inode_ptr file_ptr = directory_search(path, cwd, true);

  map<string, inode_ptr> child = file_ptr->get_lower();
  map<string,inode_ptr>::iterator it;
  it = child.find(path.at(path.size()-1));
  //Illegal path
  if(it == child.end()) {
    cout << "no file" << endl;
    return;
  }
  file_ptr = child[path.at(path.size()-1)];
  DEBUGF('r', file_ptr);

  wordvec file_data = file_ptr->contents->readfile();

  for(auto word : file_data) {
    cout << word << " ";
  }
  cout << endl;
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
    inode_ptr par = parent[name].lock();
    //cout << name << " " << parent[name].lock()->get_inode_nr() << endl;
    cout << par->get_inode_nr() << "\t" << par->size << " " << name << endl;
  }
  for(auto const &pair:children) {
    string name = pair.first;
    inode_ptr child = children[name];
    //cout << name << " " << children[name]->get_inode_nr() << endl;
    cout << child->get_inode_nr() << "\t" << child->size << " " << name << endl;

  }
}

void inode_state::listr(const wordvec& path) {
  wordvec n_path = path;
  for (auto &path_elem : path) {
    cout << "/" << path_elem << " ";
  }
  cout << endl;
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
  for(auto const &n : children) {
    string name = n.first;
    if (children[name]->type() == "d") {
      wordvec temp = split(name, "/");
      n_path.push_back(temp.at(0));
      listr(n_path);
      n_path.pop_back();
    }
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
}

void inode_state::remove_here(const wordvec& path) {
  inode_ptr curr = directory_search(path, cwd, true);
  map<string, inode_ptr> children = curr->get_lower();

  map<string,inode_ptr> :: iterator it;
  string name = path[path.size()-1];
  it = children.find(name);

  if(it!=children.end()) {
    inode_ptr temp = children[name];
    if(temp->type() == "p") {
      cout << "here"<< endl;
      children.erase(name);
    } else {
      if(temp->size == 2) {
        cout << "here" << endl;
        children.erase(name);
      }
    }
  }
  curr->set_lower(children);
}

void inode_state::remove_here_recursive(const wordvec& path) {
  inode_ptr curr = directory_search(path, cwd, true);

}

void inode_state::increment_recursive(const inode_ptr& curr) {
  if(curr == root) {
    root->size++;
    return;
  }
  map<string, inode_wk_ptr> parent = curr->get_higher();
  inode_ptr next = parent["../"].lock();
  next->size++;
  increment_recursive(next);
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

string inode::type() {
  return contents->get_type();
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

string base_file::get_type() {
  throw file_error("is a " + error_file_type());
}

/*
void base_file::increment() {
  throw file_error("is a " + error_file_type());
}*/



string plain_file::get_type() {
  return "p";
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
   DEBUGF ('r', "returning file_data: " << data);
   return data;
}

void plain_file::writefile (const wordvec& words) {
   this->data = std::move(words);
   DEBUGF ('i', words);
}

string directory::get_type() {
  return "d";
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
