// $Id: commands.cpp,v 1.20 2021-01-11 15:52:17-08 - - $
// Evan Clark, Brady Chan

#include "commands.h"
#include "debug.h"

command_hash cmd_hash {
   {"cat"   , fn_cat   },
   {"cd"    , fn_cd    },
   {"echo"  , fn_echo  },
   {"exit"  , fn_exit  },
   {"ls"    , fn_ls    },
   {"lsr"   , fn_lsr   },
   {"make"  , fn_make  },
   {"mkdir" , fn_mkdir },
   {"prompt", fn_prompt},
   {"pwd"   , fn_pwd   },
   {"rm"    , fn_rm    },
   {"rmr"   , fn_rmr   },
};

command_fn find_command_fn (const string& cmd) {
   // Note: value_type is pair<const key_type, mapped_type>
   // So: iterator->first is key_type (string)
   // So: iterator->second is mapped_type (command_fn)
   DEBUGF ('c', "[" << cmd << "]");
   const auto result = cmd_hash.find (cmd);
   if (result == cmd_hash.end()) {
      throw command_error (cmd + ": no such function");
   }
   return result->second;
}

command_error::command_error (const string& what):
            runtime_error (what) {
}

int exit_status_message() {
   int status = exec::status();
   cout << exec::execname() << ": exit(" << status << ")" << endl;
   return status;
}

void fn_cat (inode_state& state, const wordvec& words) {
   if (words.size() > 1) {
     state.print_file(words);
   } else {
     throw command_error("No file provided");
   }
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

void fn_cd (inode_state& state, const wordvec& words) {
   wordvec names;
   if(words.size() > 1) {
     names = split(words[1],"/");
   }
   state.change_directory(names);
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

void fn_echo (inode_state& state, const wordvec& words) {
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   cout << word_range (words.cbegin() + 1, words.cend()) << endl;
}


void fn_exit (inode_state& state, const wordvec& words) {   
   int given = 0;
   string deref = "";
   if(words.size() > 1) {
     deref = words.at(1);
     for(int i = 0;i < static_cast<int>(deref.size());i++) {
       if(isalpha(deref[i])) {
         given = 127;
         break;
       }
     }
     if(given != 127) {
       given = stoi(deref);
     }
   } else {
     given = state.get_errors();
   }
   exec::status(given);
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   throw ysh_exit();
}

void fn_ls (inode_state& state, const wordvec& words) {
   wordvec names;
   if(words.size() > 1) {
     if(words[1] == "/") {
       //Nothing allow root to pass
       names.push_back("/");
     } else {
       names = split(words[1],"/");
     }
   } else {
       //names.push_back(".");
   }

   /*
   if(words.size() > 1) {
     names = split(words[1],"/");
   } else {
     names.push_back(".");
   }*/
   state.list(names);
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

void fn_lsr (inode_state& state, const wordvec& words) {
   wordvec names;
   if(words.size() > 1) {
     if(words.at(1) == "/") {
        names.push_back("/");
     }else{
       names = split(words[1],"/");
     }
   } else {
     names.push_back(".");
   }
   state.listr(names);
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

void fn_make (inode_state& state, const wordvec& words) {
   if (words.size() > 1) {
    state.make_file(words);
   } else {
    throw command_error("No arguments");
   }
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

void fn_mkdir (inode_state& state, const wordvec& words) {
   wordvec names;
   if(words.size() > 1) {
     names = split(words[1],"/");
   } else {
     throw command_error("No name input");
   }
   //wordvec names = split(words[1], "/");
   state.make_directory(names);
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

void fn_prompt (inode_state& state, const wordvec& words) {
   if(words.size() > 1) {
     state.set_prompt(words);
   } else {
     throw command_error("No arguments given");
   }
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

void fn_pwd (inode_state& state, const wordvec& words) {
   state.print_working_directory();
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

void fn_rm (inode_state& state, const wordvec& words) {
   wordvec names;
   if(words.size() > 1) {
     names = split(words[1],"/");
   } else {
     throw command_error("No file name");
   }
   state.remove_here(names);
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

void fn_rmr (inode_state& state, const wordvec& words) {
   wordvec names;
   if(words.size() > 1) {
     names = split(words[1],"/");
   } else {
     throw command_error("No file name");
   }
   state.rmr(names);
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

