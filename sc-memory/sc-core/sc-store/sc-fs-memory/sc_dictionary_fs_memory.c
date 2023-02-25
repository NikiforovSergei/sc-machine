/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifdef SC_DICTIONARY_FS_MEMORY

#  include "sc_dictionary_fs_memory.h"
#  include "sc_dictionary_fs_memory_private.h"

#  include "sc_file_system.h"
#  include "sc_io.h"

#  include "../sc-base/sc_message.h"

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_initialize(
    sc_dictionary_fs_memory ** memory,
    sc_char const * path)
{
  sc_message("Sc-dictionary fs-memory: initialize");
  if (path == null_ptr)
  {
    sc_critical("Sc-dictionary fs-memory: `path` is not correct");
    return SC_FS_MEMORY_WRONG_PATH;
  }

  if (sc_fs_isdir(path) == SC_FALSE)
  {
    if (sc_fs_mkdirs(path) == SC_FALSE)
    {
      sc_critical("Sc-dictionary fs-memory: `path` is not correct");
      return SC_FS_MEMORY_WRONG_PATH;
    }
  }

  *memory = sc_mem_new(sc_dictionary_fs_memory, 1);
  {
    sc_str_cpy((*memory)->path, path, sc_str_len(path));
    (*memory)->max_searchable_string_size = 1000;

    {
      _sc_uchar_dictionary_initialize(&(*memory)->terms_string_offsets_dictionary);
      static sc_char const * term_string_offsets = "term_string_offsets" SC_FS_EXT;
      _sc_init_db_path(path, term_string_offsets, &(*memory)->terms_string_offsets_path);

      static sc_char const * strings_postfix = "strings" SC_FS_EXT;
      _sc_init_db_path(path, strings_postfix, &(*memory)->strings_path);
      (*memory)->last_string_offset = 0;
    }

    _sc_number_dictionary_initialize(&(*memory)->link_hashes_string_offsets_dictionary);
    _sc_number_dictionary_initialize(&(*memory)->string_offsets_link_hashes_dictionary);
    static sc_char const * string_offsets_link_hashes = "string_offsets_link_hashes" SC_FS_EXT;
    _sc_init_db_path(path, string_offsets_link_hashes, &(*memory)->string_offsets_link_hashes_path);
  }
  sc_message("Sc-dictionary fs-memory:");
  sc_message("\tPath: %s", path);

  return SC_FS_MEMORY_OK;
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_shutdown(sc_dictionary_fs_memory * memory)
{
  if (memory == null_ptr)
  {
    sc_message("Sc-dictionary fs-memory: memory is empty");
    return SC_FS_MEMORY_NO;
  }

  sc_message("Sc-dictionary fs-memory: shutdown");
  {
    sc_mem_free(memory->path);

    {
      sc_dictionary_destroy(memory->terms_string_offsets_dictionary, _sc_dictionary_fs_memory_node_destroy);
      sc_mem_free(memory->terms_string_offsets_path);

      sc_mem_free(memory->strings_path);
    }

    sc_dictionary_destroy(memory->link_hashes_string_offsets_dictionary, _sc_dictionary_fs_memory_string_node_destroy);
    sc_dictionary_destroy(memory->string_offsets_link_hashes_dictionary, _sc_dictionary_fs_memory_link_node_destroy);
    sc_mem_free(memory->string_offsets_link_hashes_path);
  }
  sc_mem_free(memory);

  return SC_FS_MEMORY_OK;
}

void _sc_dictionary_fs_memory_append(
    sc_dictionary * dictionary,
    sc_char const * key,
    sc_uint64 const key_size,
    void * data)
{
  sc_list * list = sc_dictionary_get_by_key(dictionary, key, key_size);
  if (list == null_ptr)
  {
    sc_char * copied_key;
    sc_str_cpy(copied_key, key, key_size);

    sc_list_init(&list);
    sc_dictionary_append(dictionary, copied_key, key_size, list);
    sc_list_push_back(list, copied_key);
  }

  sc_list_push_back(list, data);
}

sc_bool _sc_addr_hash_compare(void * addr_hash, void * other_addr_hash)
{
  return addr_hash == other_addr_hash;
}

void _sc_dictionary_fs_memory_append_link_string_unique(
    sc_dictionary_fs_memory * memory,
    sc_addr_hash const link_hash,
    sc_uint64 const string_offset)
{
  sc_char * link_hash_str;
  sc_uint64 link_hash_str_size;
  sc_bool is_content_new;
  sc_link_hash_content * content;
  {
    sc_int_to_str_int(link_hash, link_hash_str, link_hash_str_size);
    content =
        sc_dictionary_get_by_key(memory->link_hashes_string_offsets_dictionary, link_hash_str, link_hash_str_size);
    is_content_new = (content == null_ptr);
    if (is_content_new)
    {
      content = sc_mem_new(sc_link_hash_content, 1);
      sc_dictionary_append(memory->link_hashes_string_offsets_dictionary, link_hash_str, link_hash_str_size, content);
    }
  }

  sc_char * string_offset_str;
  sc_uint64 string_offset_str_size;
  sc_list * link_hashes;
  {
    sc_int_to_str_int(string_offset, string_offset_str, string_offset_str_size);
    link_hashes = sc_dictionary_get_by_key(
        memory->string_offsets_link_hashes_dictionary, string_offset_str, string_offset_str_size);
    if (link_hashes == null_ptr)
    {
      sc_list_init(&link_hashes);
      sc_dictionary_append(
          memory->string_offsets_link_hashes_dictionary, string_offset_str, string_offset_str_size, link_hashes);
    }
  }

  {
    if (!is_content_new && content->link_hashes != link_hashes)
      sc_list_remove_if(content->link_hashes, (void *)link_hash, _sc_addr_hash_compare);

    if (content->link_hashes != link_hashes)
    {
      content->string_offset = string_offset + 1;
      content->link_hashes = link_hashes;
      sc_list_push_back(content->link_hashes, (void *)link_hash);
    }
  }

  sc_mem_free(link_hash_str);
  sc_mem_free(string_offset_str);
}

void _sc_dictionary_fs_memory_get_string_offsets_by_terms(
    sc_dictionary_fs_memory const * memory,
    sc_list const * terms,
    sc_dictionary ** term_string_offsets_dictionary)
{
  _sc_uchar_dictionary_initialize(term_string_offsets_dictionary);

  sc_iterator * term_it = sc_list_iterator(terms);
  while (sc_iterator_next(term_it))
  {
    sc_char const * term = sc_iterator_get(term_it);
    sc_uint64 const term_size = sc_str_len(term);

    sc_list * string_offsets = sc_dictionary_get_by_key(memory->terms_string_offsets_dictionary, term, term_size);
    sc_iterator * string_offsets_it = sc_list_iterator(string_offsets);
    if (!sc_iterator_next(string_offsets_it))
    {
      sc_iterator_destroy(string_offsets_it);
      return;
    }

    sc_dictionary_append(*term_string_offsets_dictionary, term, term_size, string_offsets);
    sc_iterator_destroy(string_offsets_it);
  }
  sc_iterator_destroy(term_it);
}

sc_list * _sc_dictionary_fs_memory_get_string_offsets_by_term(
    sc_dictionary_fs_memory const * memory,
    sc_char const * term)
{
  sc_uint64 const term_size = sc_str_len(term);
  return sc_dictionary_get_by_key(memory->terms_string_offsets_dictionary, term, term_size);
}

sc_dictionary_fs_memory_status _sc_dictionary_node_fs_memory_get_string_offset_by_string(
    sc_io_channel * strings_channel,
    sc_char const * string,
    sc_uint64 const string_size,
    sc_list const * string_offsets,
    sc_uint64 * found_string_offset)
{
  sc_iterator * string_offset_it = sc_list_iterator(string_offsets);
  if (!sc_iterator_next(string_offset_it))
  {
    sc_iterator_destroy(string_offset_it);
    return SC_FS_MEMORY_READ_ERROR;
  }

  while (sc_iterator_next(string_offset_it))
  {
    sc_uint64 const string_offset = (sc_uint64)sc_iterator_get(string_offset_it);

    // read string with size
    sc_uint64 read_bytes;
    sc_io_channel_seek(strings_channel, string_offset, SC_FS_IO_SEEK_SET, null_ptr);
    {
      sc_uint64 other_string_size;
      if (sc_io_channel_read_chars(
              strings_channel, (sc_char *)&other_string_size, sizeof(sc_uint64), &read_bytes, null_ptr) !=
              SC_FS_IO_STATUS_NORMAL ||
          sizeof(sc_uint64) != read_bytes)
      {
        return SC_FALSE;
      }

      if (other_string_size != string_size)
        continue;

      sc_char * other_string = sc_mem_new(sc_char, other_string_size + 1);
      if (sc_io_channel_read_chars(strings_channel, other_string, other_string_size, &read_bytes, null_ptr) !=
              SC_FS_IO_STATUS_NORMAL ||
          other_string_size != read_bytes)
      {
        sc_mem_free(other_string);
        return SC_FS_MEMORY_READ_ERROR;
      }

      if (sc_str_cmp(string, other_string) == SC_FALSE)
      {
        sc_mem_free(other_string);
        continue;
      }

      sc_mem_free(other_string);
    }

    *found_string_offset = string_offset;
  }
  sc_iterator_destroy(string_offset_it);

  return SC_FS_MEMORY_OK;
}

sc_uint64 _sc_dictionary_fs_memory_get_string_offset_by_string(
    sc_dictionary_fs_memory const * memory,
    sc_io_channel * strings_channel,
    sc_char const * string,
    sc_uint64 const string_size,
    sc_char const * term)
{
  sc_list * string_offsets = _sc_dictionary_fs_memory_get_string_offsets_by_term(memory, term);

  sc_uint64 string_offset = INVALID_STRING_OFFSET;
  _sc_dictionary_node_fs_memory_get_string_offset_by_string(
      strings_channel, string, string_size, string_offsets, &string_offset);
  return string_offset;
}

sc_dictionary_fs_memory_status _sc_dictionary_fs_memory_write_string(
    sc_dictionary_fs_memory * memory,
    sc_addr_hash const link_hash,
    sc_char const * string,
    sc_uint64 const string_size,
    sc_list const * string_terms,
    sc_uint64 * string_offset)
{
  if (sc_fs_isfile(memory->strings_path) == SC_FALSE)
    sc_fs_mkfile(memory->strings_path);

  sc_io_channel * strings_channel = sc_io_new_append_channel(memory->strings_path, null_ptr);
  sc_io_channel_set_encoding(strings_channel, null_ptr, null_ptr);

  *string_offset = INVALID_STRING_OFFSET;
  if (string_size < memory->max_searchable_string_size)
    *string_offset = _sc_dictionary_fs_memory_get_string_offset_by_string(
        memory, strings_channel, string, string_size, string_terms->begin->data);

  sc_bool const to_write_string = (*string_offset == INVALID_STRING_OFFSET);
  if (to_write_string)
  {
    *string_offset = memory->last_string_offset;
    sc_io_channel_seek(strings_channel, *string_offset, SC_FS_IO_SEEK_SET, null_ptr);
  }

  // cache string offset and link hash data
  {
    _sc_dictionary_fs_memory_append_link_string_unique(memory, link_hash, *string_offset);
  }

  // save string in db
  if (to_write_string)
  {
    sc_uint64 written_bytes = 0;
    if (sc_io_channel_write_chars(strings_channel, &string_size, sizeof(string_size), &written_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(string_size) != written_bytes)
    {
      sc_critical("Sc-dictionary fs-memory: error while `size` writing");
      sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);
      return SC_FS_MEMORY_WRITE_ERROR;
    }

    memory->last_string_offset += written_bytes;

    if (sc_io_channel_write_chars(strings_channel, string, string_size, &written_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        string_size != written_bytes)
    {
      sc_critical("Sc-dictionary fs-memory: error while `string` writing");
      sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);
      return SC_FS_MEMORY_WRITE_ERROR;
    }

    memory->last_string_offset += written_bytes;
  }

  sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);

  return SC_FS_MEMORY_OK;
}

sc_dictionary_fs_memory_status _sc_dictionary_fs_memory_write_string_terms_string_offset(
    sc_dictionary_fs_memory * memory,
    sc_uint64 const string_offset,
    sc_list * string_terms)
{
  sc_iterator * term_it = sc_list_iterator(string_terms);
  while (sc_iterator_next(term_it))
  {
    sc_char * term = sc_iterator_get(term_it);
    sc_uint64 const term_size = sc_str_len(term);

    // cache term offset
    {
      _sc_dictionary_fs_memory_append(memory->terms_string_offsets_dictionary, term, term_size, (void *)string_offset);
    }

    sc_mem_free(term);
  }
  sc_iterator_destroy(term_it);

  return SC_FS_MEMORY_OK;
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_link_string(
    sc_dictionary_fs_memory * memory,
    sc_addr_hash const link_hash,
    sc_char const * string,
    sc_uint64 const string_size)
{
  if (memory == null_ptr)
  {
    sc_critical("Sc-dictionary fs-memory: memory is empty to link string");
    return SC_FS_MEMORY_NO;
  }

  sc_list * string_terms;
  if (string_size < memory->max_searchable_string_size)
    string_terms = _sc_dictionary_fs_memory_get_string_terms(string);

  sc_uint64 string_offset;
  sc_dictionary_fs_memory_status status =
      _sc_dictionary_fs_memory_write_string(memory, link_hash, string, string_size, string_terms, &string_offset);
  if (status != SC_FS_MEMORY_OK)
    return status;

  if (string_size < memory->max_searchable_string_size)
  {
    status = _sc_dictionary_fs_memory_write_string_terms_string_offset(memory, string_offset, string_terms);
    sc_list_destroy(string_terms);
  }

  return status;
}

sc_dictionary_fs_memory_status _sc_dictionary_fs_memory_read_string_by_offset(
    sc_dictionary_fs_memory const * memory,
    sc_uint64 const string_offset,
    sc_char ** string)
{
  sc_io_channel * strings_channel = sc_io_new_read_channel(memory->strings_path, null_ptr);
  if (strings_channel == null_ptr)
  {
    sc_critical("Sc-dictionary fs-memory: `strings_path` doesn't exist");
    return SC_FS_MEMORY_READ_ERROR;
  }

  sc_io_channel_set_encoding(strings_channel, null_ptr, null_ptr);

  // read string with size
  sc_uint64 read_bytes;
  sc_io_channel_seek(strings_channel, string_offset, SC_FS_IO_SEEK_SET, null_ptr);
  {
    sc_uint64 string_size;
    if (sc_io_channel_read_chars(strings_channel, (sc_char *)&string_size, sizeof(sc_uint64), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_uint64) != read_bytes)
    {
      *string = null_ptr;
      sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);
      return SC_FS_MEMORY_READ_ERROR;
    }

    *string = sc_mem_new(sc_char, string_size + 1);
    if (sc_io_channel_read_chars(strings_channel, *string, string_size, &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        string_size != read_bytes)
    {
      sc_mem_free(string);
      *string = null_ptr;
      sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);
      return SC_FS_MEMORY_READ_ERROR;
    }
  }

  sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);

  return SC_FS_MEMORY_OK;
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_get_string_by_link_hash(
    sc_dictionary_fs_memory const * memory,
    sc_addr_hash const link_hash,
    sc_char ** string,
    sc_uint64 * string_size)
{
  sc_char * link_hash_str;
  sc_uint64 link_hash_str_size;
  sc_int_to_str_int(link_hash, link_hash_str, link_hash_str_size);
  sc_link_hash_content * content =
      sc_dictionary_get_by_key(memory->link_hashes_string_offsets_dictionary, link_hash_str, link_hash_str_size);
  sc_mem_free(link_hash_str);

  if (content == null_ptr)
  {
    *string = null_ptr;
    *string_size = 0;
    return SC_FS_MEMORY_NO_STRING;
  }

  sc_uint64 const string_offset = (sc_uint64)content->string_offset - 1;
  sc_dictionary_fs_memory_status const status =
      _sc_dictionary_fs_memory_read_string_by_offset(memory, string_offset, string);
  if (status != SC_FS_MEMORY_OK)
  {
    *string = null_ptr;
    *string_size = 0;
    return SC_FS_MEMORY_READ_ERROR;
  }

  *string_size = sc_str_len(*string);
  return SC_FS_MEMORY_OK;
}

sc_dictionary_fs_memory_status _sc_dictionary_fs_memory_get_link_hashes_by_string_term(
    sc_dictionary_fs_memory const * memory,
    sc_char const * string,
    sc_uint64 const string_size,
    sc_bool const is_substring,
    sc_list const * string_offsets,
    sc_list ** link_hashes)
{
  sc_list_init(link_hashes);

  sc_io_channel * strings_channel = sc_io_new_read_channel(memory->strings_path, null_ptr);
  if (strings_channel == null_ptr)
  {
    sc_critical("Sc-dictionary fs-memory: `strings_path` doesn't exist");
    return SC_FS_MEMORY_READ_ERROR;
  }

  sc_io_channel_set_encoding(strings_channel, null_ptr, null_ptr);

  sc_iterator * string_offset_it = sc_list_iterator(string_offsets);
  if (!sc_iterator_next(string_offset_it))
    return SC_FS_MEMORY_READ_ERROR;

  while (sc_iterator_next(string_offset_it))
  {
    sc_uint64 const string_offset = (sc_uint64)sc_iterator_get(string_offset_it);

    // read string with size
    sc_uint64 read_bytes;
    sc_io_channel_seek(strings_channel, string_offset, SC_FS_IO_SEEK_SET, null_ptr);
    {
      sc_uint64 other_string_size;
      if (sc_io_channel_read_chars(
              strings_channel, (sc_char *)&other_string_size, sizeof(sc_uint64), &read_bytes, null_ptr) !=
              SC_FS_IO_STATUS_NORMAL ||
          sizeof(sc_uint64) != read_bytes)
      {
        sc_iterator_destroy(string_offset_it);
        sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);
        return SC_FS_MEMORY_READ_ERROR;
      }

      if ((is_substring && other_string_size < string_size) || (!is_substring && other_string_size != string_size))
        continue;

      sc_char * other_string = sc_mem_new(sc_char, other_string_size + 1);
      if (sc_io_channel_read_chars(strings_channel, other_string, other_string_size, &read_bytes, null_ptr) !=
              SC_FS_IO_STATUS_NORMAL ||
          other_string_size != read_bytes)
      {
        sc_mem_free(other_string);
        sc_iterator_destroy(string_offset_it);
        sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);
        return SC_FS_MEMORY_READ_ERROR;
      }

      if ((is_substring && sc_str_find(other_string, string) == SC_FALSE) ||
          (!is_substring && sc_str_cmp(string, other_string) == SC_FALSE))
      {
        sc_mem_free(other_string);
        continue;
      }

      sc_mem_free(other_string);
    }

    sc_char * string_offset_str;
    sc_uint64 string_offset_str_size;
    sc_int_to_str_int(string_offset, string_offset_str, string_offset_str_size);

    sc_list * data = sc_dictionary_get_by_key(
        memory->string_offsets_link_hashes_dictionary, string_offset_str, string_offset_str_size);
    sc_mem_free(string_offset_str);

    sc_iterator * data_it = sc_list_iterator(data);
    while (sc_iterator_next(data_it))
    {
      void * link_hash = sc_iterator_get(data_it);
      sc_list_push_back(*link_hashes, link_hash);
    }
    sc_iterator_destroy(data_it);
  }
  sc_iterator_destroy(string_offset_it);

  sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);

  return SC_FS_MEMORY_OK;
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_get_link_hashes_by_string_ext(
    sc_dictionary_fs_memory const * memory,
    sc_char const * string,
    sc_uint64 const string_size,
    sc_bool const is_substring,
    sc_list ** link_hashes)
{
  sc_char * term = _sc_dictionary_fs_memory_get_first_term(string);
  sc_list * string_offsets = _sc_dictionary_fs_memory_get_string_offsets_by_term(memory, term);
  sc_mem_free(term);

  sc_dictionary_fs_memory_status const status = _sc_dictionary_fs_memory_get_link_hashes_by_string_term(
      memory, string, string_size, is_substring, string_offsets, link_hashes);
  return status;
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_get_link_hashes_by_string(
    sc_dictionary_fs_memory const * memory,
    sc_char const * string,
    sc_uint64 const string_size,
    sc_list ** link_hashes)
{
  return sc_dictionary_fs_memory_get_link_hashes_by_string_ext(memory, string, string_size, SC_FALSE, link_hashes);
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_get_link_hashes_by_substring(
    sc_dictionary_fs_memory const * memory,
    sc_char const * string,
    sc_uint64 const string_size,
    sc_list ** link_hashes)
{
  return sc_dictionary_fs_memory_get_link_hashes_by_string_ext(memory, string, string_size, SC_TRUE, link_hashes);
}

sc_dictionary_fs_memory_status _sc_dictionary_fs_memory_get_strings_by_substring_term(
    sc_dictionary_fs_memory const * memory,
    sc_char const * string,
    sc_uint64 const string_size,
    sc_list const * string_offsets,
    sc_list ** strings)
{
  sc_list_init(strings);

  sc_io_channel * strings_channel = sc_io_new_read_channel(memory->strings_path, null_ptr);
  if (strings_channel == null_ptr)
  {
    sc_critical("Sc-dictionary fs-memory: `strings_path` doesn't exist");
    return SC_FS_MEMORY_READ_ERROR;
  }

  sc_io_channel_set_encoding(strings_channel, null_ptr, null_ptr);

  sc_iterator * string_offset_it = sc_list_iterator(string_offsets);
  if (!sc_iterator_next(string_offset_it))
    return SC_FS_MEMORY_READ_ERROR;

  while (sc_iterator_next(string_offset_it))
  {
    sc_uint64 const string_offset = (sc_uint64)sc_iterator_get(string_offset_it);

    // read string with size
    sc_uint64 read_bytes;
    sc_io_channel_seek(strings_channel, string_offset, SC_FS_IO_SEEK_SET, null_ptr);
    {
      sc_uint64 other_string_size;
      if (sc_io_channel_read_chars(
              strings_channel, (sc_char *)&other_string_size, sizeof(sc_uint64), &read_bytes, null_ptr) !=
              SC_FS_IO_STATUS_NORMAL ||
          sizeof(sc_uint64) != read_bytes)
      {
        sc_iterator_destroy(string_offset_it);
        sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);
        return SC_FS_MEMORY_READ_ERROR;
      }

      if (other_string_size < string_size)
        continue;

      sc_char * other_string = sc_mem_new(sc_char, other_string_size + 1);
      if (sc_io_channel_read_chars(strings_channel, other_string, other_string_size, &read_bytes, null_ptr) !=
              SC_FS_IO_STATUS_NORMAL ||
          other_string_size != read_bytes)
      {
        sc_mem_free(other_string);
        sc_iterator_destroy(string_offset_it);
        sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);
        return SC_FS_MEMORY_READ_ERROR;
      }

      if (sc_str_find(other_string, string) == SC_FALSE)
      {
        sc_mem_free(other_string);
        continue;
      }

      sc_list_push_back(*strings, other_string);
    }
  }
  sc_iterator_destroy(string_offset_it);

  sc_io_channel_shutdown(strings_channel, SC_TRUE, null_ptr);

  return SC_FS_MEMORY_OK;
}

sc_bool _sc_dictionary_fs_memory_visit_string_offsets_by_term_prefix(sc_dictionary_node * node, void ** arguments)
{
  if (node->data == null_ptr)
    return SC_TRUE;

  sc_list * string_offsets = arguments[0];
  sc_iterator * it = sc_list_iterator(node->data);
  if (!sc_iterator_next(it))
  {
    sc_iterator_destroy(it);
    return SC_TRUE;
  }

  while (sc_iterator_next(it))
  {
    sc_list_push_back(string_offsets, sc_iterator_get(it));
  }
  sc_iterator_destroy(it);

  return SC_TRUE;
}

sc_list * _sc_dictionary_fs_memory_get_string_offsets_by_term_prefix(
    sc_dictionary_fs_memory const * memory,
    sc_char const * term)
{
  sc_uint64 const term_size = sc_str_len(term);
  sc_list * string_offsets;
  sc_list_init(&string_offsets);
  sc_list_push_back(string_offsets, null_ptr);

  sc_dictionary_get_by_key_prefix(
      memory->terms_string_offsets_dictionary,
      term,
      term_size,
      _sc_dictionary_fs_memory_visit_string_offsets_by_term_prefix,
      (void **)&string_offsets);

  return string_offsets;
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_get_strings_by_substring_ext(
    sc_dictionary_fs_memory const * memory,
    sc_char const * string,
    sc_uint64 const string_size,
    sc_list ** strings)
{
  sc_char * term = _sc_dictionary_fs_memory_get_first_term(string);
  sc_list * string_offsets = _sc_dictionary_fs_memory_get_string_offsets_by_term_prefix(memory, term);
  sc_mem_free(term);

  sc_dictionary_fs_memory_status const status =
      _sc_dictionary_fs_memory_get_strings_by_substring_term(memory, string, string_size, string_offsets, strings);
  return status;
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_get_strings_by_substring(
    sc_dictionary_fs_memory const * memory,
    sc_char const * string,
    sc_uint64 const string_size,
    sc_list ** strings)
{
  return sc_dictionary_fs_memory_get_strings_by_substring_ext(memory, string, string_size, strings);
}

sc_bool _sc_dictionary_fs_memory_get_link_hashes_by_string_offsets(sc_dictionary_node * node, void ** arguments)
{
  if (node->data == null_ptr)
    return SC_TRUE;

  sc_dictionary_fs_memory * memory = arguments[0];
  sc_uint64 const size = (sc_uint64)arguments[1];
  sc_list * link_hashes = arguments[2];

  sc_list * list = node->data;
  // unite or intersect link hashes
  if (size == 0 || list->size == size + 1)
  {
    sc_uint64 const string_offset = (sc_uint64)list->begin->next->data;
    sc_char * string_offset_str;
    sc_uint64 string_offset_str_size;
    sc_int_to_str_int(string_offset, string_offset_str, string_offset_str_size);

    sc_list * data = sc_dictionary_get_by_key(
        memory->string_offsets_link_hashes_dictionary, string_offset_str, string_offset_str_size);
    sc_iterator * data_it = sc_list_iterator(data);
    while (sc_iterator_next(data_it))
    {
      void * hash = sc_iterator_get(data_it);
      sc_list_push_back(link_hashes, hash);
    }
    sc_iterator_destroy(data_it);
  }

  return SC_TRUE;
}

sc_dictionary_fs_memory_status _sc_dictionary_fs_memory_get_link_hashes_by_terms(
    sc_dictionary_fs_memory const * memory,
    sc_list const * terms,
    sc_bool const intersect,
    sc_list ** link_hashes)
{
  sc_list_init(link_hashes);
  if (terms->size == 0)
    return SC_FS_MEMORY_OK;

  sc_dictionary * term_offsets_dictionary;
  _sc_dictionary_fs_memory_get_string_offsets_by_terms(memory, terms, &term_offsets_dictionary);

  void * arguments[3];
  arguments[0] = (void *)memory;
  arguments[1] = intersect ? (void *)(sc_uint64)terms->size : 0;
  arguments[2] = *link_hashes;
  sc_dictionary_fs_memory_status const status = sc_dictionary_visit_down_nodes(
      term_offsets_dictionary, _sc_dictionary_fs_memory_get_link_hashes_by_string_offsets, arguments);
  sc_dictionary_destroy(term_offsets_dictionary, _sc_dictionary_fs_memory_node_destroy) ? SC_FS_MEMORY_OK
                                                                                        : SC_FS_MEMORY_READ_ERROR;
  return status;
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_intersect_link_hashes_by_terms(
    sc_dictionary_fs_memory const * memory,
    sc_list const * terms,
    sc_list ** link_hashes)
{
  return _sc_dictionary_fs_memory_get_link_hashes_by_terms(memory, terms, SC_TRUE, link_hashes);
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_unite_link_hashes_by_terms(
    sc_dictionary_fs_memory const * memory,
    sc_list const * terms,
    sc_list ** link_hashes)
{
  return _sc_dictionary_fs_memory_get_link_hashes_by_terms(memory, terms, SC_FALSE, link_hashes);
}

sc_bool _sc_dictionary_fs_memory_get_string_by_string_offsets(sc_dictionary_node * node, void ** arguments)
{
  if (node->data == null_ptr)
    return SC_TRUE;

  sc_dictionary_fs_memory * memory = arguments[0];
  sc_uint64 const size = (sc_uint64)arguments[1];
  sc_list * strings = arguments[2];

  sc_list * list = node->data;
  // unite or intersect strings
  if (size == 0 || list->size == size + 1)
  {
    sc_uint64 const string_offset = (sc_uint64)list->begin->next->data;
    sc_char * string;
    if (_sc_dictionary_fs_memory_read_string_by_offset(memory, string_offset, &string) != SC_FS_MEMORY_OK)
      return SC_FALSE;

    sc_list_push_back(strings, string);
  }

  return SC_TRUE;
}

sc_dictionary_fs_memory_status _sc_dictionary_fs_memory_get_strings_by_terms(
    sc_dictionary_fs_memory const * memory,
    sc_list const * terms,
    sc_bool const intersect,
    sc_list ** strings)
{
  sc_list_init(strings);
  if (terms->size == 0)
    return SC_FS_MEMORY_OK;

  sc_dictionary * term_string_offsets_dictionary;
  _sc_dictionary_fs_memory_get_string_offsets_by_terms(memory, terms, &term_string_offsets_dictionary);

  void * arguments[3];
  arguments[0] = (void *)memory;
  arguments[1] = intersect ? (void *)(sc_uint64)terms->size : 0;
  arguments[2] = *strings;
  sc_dictionary_visit_down_nodes(
      term_string_offsets_dictionary, _sc_dictionary_fs_memory_get_string_by_string_offsets, arguments);
  sc_dictionary_destroy(term_string_offsets_dictionary, sc_dictionary_node_destroy);

  return SC_FS_MEMORY_OK;
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_intersect_strings_by_terms(
    sc_dictionary_fs_memory const * memory,
    sc_list const * terms,
    sc_list ** strings)
{
  return _sc_dictionary_fs_memory_get_strings_by_terms(memory, terms, SC_TRUE, strings);
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_unite_strings_by_terms(
    sc_dictionary_fs_memory const * memory,
    sc_list const * terms,
    sc_list ** strings)
{
  return _sc_dictionary_fs_memory_get_strings_by_terms(memory, terms, SC_FALSE, strings);
}

void _sc_dictionary_fs_memory_read_terms_string_offsets(sc_dictionary_fs_memory * memory, sc_io_channel * channel)
{
  sc_uint64 read_bytes = 0;
  while (SC_TRUE)
  {
    sc_uint64 term_size;
    if (sc_io_channel_read_chars(channel, (sc_char *)&term_size, sizeof(sc_uint64), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_uint64) != read_bytes)
      break;

    sc_char term[term_size + 1];
    if (sc_io_channel_read_chars(channel, (sc_char *)term, term_size, &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        term_size != read_bytes)
      break;
    term[term_size] = '\0';

    sc_uint64 term_offset_count;
    if (sc_io_channel_read_chars(channel, (sc_char *)&term_offset_count, sizeof(sc_uint64), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_uint64) != read_bytes)
      break;

    for (sc_uint64 i = 0; i < term_offset_count; ++i)
    {
      sc_uint64 string_offset;
      if (sc_io_channel_read_chars(channel, (sc_char *)&string_offset, sizeof(sc_uint64), &read_bytes, null_ptr) !=
              SC_FS_IO_STATUS_NORMAL ||
          sizeof(sc_uint64) != read_bytes)
        break;

      _sc_dictionary_fs_memory_append(memory->terms_string_offsets_dictionary, term, term_size, (void *)string_offset);
    }
  }
}

sc_dictionary_fs_memory_status _sc_dictionary_fs_memory_load_terms_offsets(sc_dictionary_fs_memory * memory)
{
  sc_io_channel * terms_offsets_channel = sc_io_new_read_channel(memory->terms_string_offsets_path, null_ptr);
  if (terms_offsets_channel == null_ptr)
  {
    sc_critical("Sc-dictionary fs-memory: `terms_string_offsets_path` doesn't exist");
    return SC_FS_MEMORY_READ_ERROR;
  }

  sc_io_channel_set_encoding(terms_offsets_channel, null_ptr, null_ptr);

  sc_uint64 read_bytes = 0;
  if (sc_io_channel_read_chars(
          terms_offsets_channel, (sc_char *)&memory->last_string_offset, sizeof(sc_uint64), &read_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_uint64) != read_bytes)
  {
    sc_critical("Sc-dictionary fs-memory: error while `last_string_offset` reading");
    sc_io_channel_shutdown(terms_offsets_channel, SC_TRUE, null_ptr);
    return SC_FS_MEMORY_READ_ERROR;
  }

  _sc_dictionary_fs_memory_read_terms_string_offsets(memory, terms_offsets_channel);

  sc_io_channel_shutdown(terms_offsets_channel, SC_TRUE, null_ptr);

  sc_message("Sc-dictionary fs-memory: `term - offsets` read");
  return SC_FS_MEMORY_OK;
}

void _sc_dictionary_fs_memory_read_string_offsets_link_hashes(sc_dictionary_fs_memory * memory, sc_io_channel * channel)
{
  sc_uint64 read_bytes = 0;
  while (SC_TRUE)
  {
    sc_uint64 string_offset;
    if (sc_io_channel_read_chars(channel, (sc_char *)&string_offset, sizeof(sc_uint64), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_uint64) != read_bytes)
      break;

    sc_uint64 link_hashes_count;
    if (sc_io_channel_read_chars(channel, (sc_char *)&link_hashes_count, sizeof(sc_uint64), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_uint64) != read_bytes)
      break;

    for (sc_uint64 i = 0; i < link_hashes_count; ++i)
    {
      sc_addr_hash link_hash;
      if (sc_io_channel_read_chars(channel, (sc_char *)&link_hash, sizeof(sc_addr_hash), &read_bytes, null_ptr) !=
              SC_FS_IO_STATUS_NORMAL ||
          sizeof(sc_addr_hash) != read_bytes)
        break;

      _sc_dictionary_fs_memory_append_link_string_unique(memory, link_hash, string_offset);
    }
  }
}

sc_dictionary_fs_memory_status _sc_dictionary_fs_memory_load_string_offsets_link_hashes(
    sc_dictionary_fs_memory * memory)
{
  sc_io_channel * channel = sc_io_new_read_channel(memory->string_offsets_link_hashes_path, null_ptr);
  if (channel == null_ptr)
  {
    sc_critical("Sc-dictionary fs-memory: `string_offsets_link_hashes_path` doesn't exist");
    return SC_FS_MEMORY_READ_ERROR;
  }
  sc_io_channel_set_encoding(channel, null_ptr, null_ptr);

  _sc_dictionary_fs_memory_read_string_offsets_link_hashes(memory, channel);

  sc_io_channel_shutdown(channel, SC_TRUE, null_ptr);
  sc_message("Sc-dictionary fs-memory: `string offsets - link hashes` read");

  return SC_FS_MEMORY_OK;
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_load(sc_dictionary_fs_memory * memory)
{
  sc_message("Sc-dictionary fs-memory: load data");
  sc_dictionary_fs_memory_status status = _sc_dictionary_fs_memory_load_terms_offsets(memory);
  if (status != SC_FS_MEMORY_OK)
    return status;
  sc_message("\tLast string offset: %lld", memory->last_string_offset);

  status = _sc_dictionary_fs_memory_load_string_offsets_link_hashes(memory);
  if (status != SC_FS_MEMORY_OK)
    return status;

  sc_message("Sc-dictionary fs-memory: all data loaded");

  return SC_FS_MEMORY_OK;
}

sc_bool _sc_dictionary_fs_memory_write_term_string_offsets(sc_dictionary_node * node, void ** arguments)
{
  if (node->data == null_ptr)
    return SC_TRUE;

  sc_io_channel * channel = arguments[0];

  sc_list * list = node->data;
  sc_iterator * data_it = sc_list_iterator(list);

  // save term in db
  sc_uint64 written_bytes = 0;
  if (sc_iterator_next(data_it))
  {
    sc_char * term = sc_iterator_get(data_it);
    sc_uint64 const term_size = sc_str_len(term);
    if (sc_io_channel_write_chars(channel, (sc_char *)&term_size, sizeof(sc_uint64), &written_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_uint64) != written_bytes)
    {
      sc_critical("Sc-dictionary fs-memory: error while `term_size` writing");
      sc_io_channel_shutdown(channel, SC_TRUE, null_ptr);
      sc_iterator_destroy(data_it);
      return SC_FALSE;
    }

    if (sc_io_channel_write_chars(channel, (sc_char *)term, term_size, &written_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        term_size != written_bytes)
    {
      sc_mem_free(term);
      sc_critical("Sc-dictionary fs-memory: error while `term` writing");
      sc_io_channel_shutdown(channel, SC_TRUE, null_ptr);
      sc_iterator_destroy(data_it);
      return SC_FALSE;
    }

    sc_uint64 const term_offsets_count = list->size - 1;
    if (sc_io_channel_write_chars(
            channel, (sc_char *)&term_offsets_count, sizeof(sc_uint64), &written_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_uint64) != written_bytes)
    {
      sc_mem_free(term);
      sc_critical("Sc-dictionary fs-memory: error while `term_offsets_count` writing");
      sc_io_channel_shutdown(channel, SC_TRUE, null_ptr);
      sc_iterator_destroy(data_it);
      return SC_FALSE;
    }

    while (sc_iterator_next(data_it))
    {
      sc_uint64 const string_offset = (sc_uint64)sc_iterator_get(data_it);
      if (sc_io_channel_write_chars(
              channel, (sc_char *)&string_offset, sizeof(string_offset), &written_bytes, null_ptr) !=
              SC_FS_IO_STATUS_NORMAL ||
          sizeof(sc_uint64) != written_bytes)
      {
        sc_mem_free(term);
        sc_critical("Sc-dictionary fs-memory: error while `string_offset` writing");
        sc_io_channel_shutdown(channel, SC_TRUE, null_ptr);
        sc_iterator_destroy(data_it);
        return SC_FALSE;
      }
    }
  }
  sc_iterator_destroy(data_it);

  return SC_TRUE;
}

sc_dictionary_fs_memory_status _sc_dictionary_fs_memory_save_term_string_offsets(sc_dictionary_fs_memory const * memory)
{
  if (sc_fs_isfile(memory->terms_string_offsets_path) == SC_FALSE)
    sc_fs_mkfile(memory->terms_string_offsets_path);

  sc_io_channel * channel = sc_io_new_write_channel(memory->terms_string_offsets_path, null_ptr);
  sc_io_channel_set_encoding(channel, null_ptr, null_ptr);

  sc_uint64 written_bytes = 0;
  if (sc_io_channel_write_chars(
          channel, (sc_char *)&memory->last_string_offset, sizeof(sc_uint64), &written_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_uint64) != written_bytes)
  {
    sc_critical("Sc-dictionary fs-memory: error while `last_string_offset` writing");
    sc_io_channel_shutdown(channel, SC_TRUE, null_ptr);
    return SC_FS_MEMORY_WRITE_ERROR;
  }

  if (!sc_dictionary_visit_down_nodes(
          memory->terms_string_offsets_dictionary,
          _sc_dictionary_fs_memory_write_term_string_offsets,
          (void **)&channel))
    return SC_FS_MEMORY_WRITE_ERROR;

  sc_io_channel_shutdown(channel, SC_TRUE, null_ptr);

  sc_message("Sc-dictionary fs-memory: `term - offsets` written");
  return SC_FS_MEMORY_OK;
}

sc_bool _sc_dictionary_fs_memory_write_string_offsets_link_hashes(sc_dictionary_node * node, void ** arguments)
{
  if (node->data == null_ptr)
    return SC_TRUE;

  sc_io_channel * channel = arguments[0];

  sc_link_hash_content * content = node->data;
  sc_iterator * data_it = sc_list_iterator(content->link_hashes);

  sc_uint64 written_bytes = 0;
  sc_uint64 const string_offset = content->string_offset - 1;
  if (sc_io_channel_write_chars(channel, (sc_char *)&string_offset, sizeof(sc_uint64), &written_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_uint64) != written_bytes)
  {
    sc_critical("Sc-dictionary fs-memory: error while `string_offset` writing");
    sc_io_channel_shutdown(channel, SC_TRUE, null_ptr);
    sc_iterator_destroy(data_it);
    return SC_FALSE;
  }

  sc_uint64 const link_hashes_count = content->link_hashes->size;
  if (sc_io_channel_write_chars(channel, (sc_char *)&link_hashes_count, sizeof(sc_uint64), &written_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_uint64) != written_bytes)
  {
    sc_critical("Sc-dictionary fs-memory: error while `link_hashes_count` writing");
    sc_io_channel_shutdown(channel, SC_TRUE, null_ptr);
    sc_iterator_destroy(data_it);
    return SC_FALSE;
  }

  while (sc_iterator_next(data_it))
  {
    sc_addr_hash const link_hash = (sc_uint64)sc_iterator_get(data_it);
    if (sc_io_channel_write_chars(channel, (sc_char *)&link_hash, sizeof(sc_addr_hash), &written_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_addr_hash) != written_bytes)
    {
      sc_critical("Sc-dictionary fs-memory: error while `link_hash` writing");
      sc_io_channel_shutdown(channel, SC_TRUE, null_ptr);
      sc_iterator_destroy(data_it);
      return SC_FALSE;
    }
  }
  sc_iterator_destroy(data_it);

  return SC_TRUE;
}

sc_dictionary_fs_memory_status _sc_dictionary_fs_memory_save_string_offsets_link_hashes(
    sc_dictionary_fs_memory const * memory)
{
  if (sc_fs_isfile(memory->string_offsets_link_hashes_path) == SC_FALSE)
    sc_fs_mkfile(memory->string_offsets_link_hashes_path);

  sc_io_channel * channel = sc_io_new_write_channel(memory->string_offsets_link_hashes_path, null_ptr);
  sc_io_channel_set_encoding(channel, null_ptr, null_ptr);

  if (!sc_dictionary_visit_down_nodes(
          memory->link_hashes_string_offsets_dictionary,
          _sc_dictionary_fs_memory_write_string_offsets_link_hashes,
          (void **)&channel))
    return SC_FS_MEMORY_WRITE_ERROR;

  sc_io_channel_shutdown(channel, SC_TRUE, null_ptr);

  sc_message("Sc-dictionary fs-memory: `string offsets - link hashes` written");
  return SC_FS_MEMORY_OK;
}

sc_dictionary_fs_memory_status sc_dictionary_fs_memory_save(sc_dictionary_fs_memory const * memory)
{
  sc_message("Sc-dictionary fs-memory: save data");
  sc_dictionary_fs_memory_status status = _sc_dictionary_fs_memory_save_term_string_offsets(memory);
  if (status != SC_FS_MEMORY_OK)
    return status;

  status = _sc_dictionary_fs_memory_save_string_offsets_link_hashes(memory);
  if (status != SC_FS_MEMORY_OK)
    return status;

  sc_message("Sc-dictionary fs-memory: all data saved");
  return status;
}

#endif
