#include "DialogListWindow.hpp"
#include "ChatSearchWindow.hpp"
#include "StickerManager.hpp"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/overloaded.h"
#include "TdObjectsOutput.h"
#include "td/utils/Random.h"
#include "td/utils/Status.h"
#include "ChatInfoWindow.hpp"
#include "FolderSelectionWindow.hpp"
#include "windows/Output.hpp"
#include <memory>
#include <vector>

namespace tdcurses {

td::int32 DialogListWindow::Element::render(windows::PadWindow &root, windows::WindowOutputter &rb,
                                            windows::SavedRenderedImagesDirectory &dir, bool is_selected) {
  auto &dialog_list_window = static_cast<DialogListWindow &>(root);
  Outputter out;
  std::string prefix;
  std::vector<td::int64> unknown_custom_emoji_ids;
  if (unread_count() > 0) {
    if (is_muted()) {
      out << Color::Grey << (unread_count() > 9 ? 9 : unread_count()) << Color::Revert << " ";
    } else {
      out << Color::Red << (unread_count() > 9 ? 9 : unread_count()) << Color::Revert << " ";
    }
  } else if (is_marked_unread()) {
    out << Color::Red << "\xe2\x80\xa2" << Color::Revert << " ";
  } else {
    out << "  ";
  }
  if (is_pinned(dialog_list_window.cur_sublist())) {
    out << "🔒";
  }
  out << title();
  if (chat_type() == ChatType::User) {
    auto user = chat_manager().get_user(this->chat_base_id());
    if (user && user->emoji_status()) {
      auto &emoji_status = user->emoji_status();
      td::td_api::downcast_call(const_cast<td::td_api::EmojiStatusType &>(*emoji_status->type_),
                                td::overloaded(
                                    [&](td::td_api::emojiStatusTypeCustomEmoji &e) {
                                      auto emoji_id = e.custom_emoji_id_;
                                      auto S = sticker_manager().get_custom_emoji(emoji_id);
                                      if (S.size() > 0) {
                                        out << sticker_manager().get_custom_emoji(emoji_id);
                                      } else {
                                        unknown_custom_emoji_ids.push_back(emoji_id);
                                      }
                                    },
                                    [&](td::td_api::emojiStatusTypeUpgradedGift &e) {
                                      auto emoji_id = e.model_custom_emoji_id_;
                                      auto S = sticker_manager().get_custom_emoji(emoji_id);
                                      if (S.size() > 0) {
                                        out << sticker_manager().get_custom_emoji(emoji_id);
                                      } else {
                                        unknown_custom_emoji_ids.push_back(emoji_id);
                                      }
                                    }));
    }
  }
  if (unknown_custom_emoji_ids.size() > 0) {
    if (unknown_custom_emoji_ids.size() > 200) {
      unknown_custom_emoji_ids.resize(200);
    }
    dialog_list_window.send_request(
        td::make_tl_object<td::td_api::getCustomEmojiStickers>(std::move(unknown_custom_emoji_ids)),
        [window = &dialog_list_window](td::Result<td::tl_object_ptr<td::td_api::stickers>> R) {
          if (R.is_ok()) {
            sticker_manager().process_custom_emoji_stickers(*R.move_as_ok());
            window->set_need_refresh();
          }
        });
  }
  return render_plain_text(rb, out.as_cslice(), out.markup(), width(), 1, is_selected, &dir);
}

bool DialogListWindow::Element::is_pinned(const DialogListWindow::Sublist &cur_sublist) {
  bool res = false;
  cur_sublist.visit(td::overloaded(
      [&](const SublistGlobal &sublist) {
        const auto x = get_order_full(td::td_api::chatListMain());
        res = x && x->is_pinned_;
      },
      [&](const SublistArchive &sublist) {
        const auto x = get_order_full(td::td_api::chatListArchive());
        res = x && x->is_pinned_;
      },
      [&](const SublistSublist &sublist) {
        const auto x = get_order_full(td::td_api::chatListFolder(sublist.sublist_id_));
        res = x && x->is_pinned_;
      },
      [&](const SublistSearch &sublist) {}));
  return res;
}

void DialogListWindow::Element::update_order(const DialogListWindow::Sublist &cur_sublist) {
  cur_sublist.visit(td::overloaded(
      [&](const SublistGlobal &sublist) { order_ = get_order(td::td_api::chatListMain()); },
      [&](const SublistArchive &sublist) { order_ = get_order(td::td_api::chatListArchive()); },
      [&](const SublistSublist &sublist) { order_ = get_order(td::td_api::chatListFolder(sublist.sublist_id_)); },
      [&](const SublistSearch &sublist) {}));
}

void DialogListWindow::Element::update_sublist(const DialogListWindow::Sublist &cur_sublist) {
  cur_sublist.visit(td::overloaded(
      [&](const SublistGlobal &sublist) { order_ = get_order(td::td_api::chatListMain()); },
      [&](const SublistArchive &sublist) { order_ = get_order(td::td_api::chatListArchive()); },
      [&](const SublistSublist &sublist) { order_ = get_order(td::td_api::chatListFolder(sublist.sublist_id_)); },
      [&](const SublistSearch &sublist) { order_ = 0; }));
}

void DialogListWindow::request_bottom_elements() {
  if (running_req_ || is_completed_) {
    return;
  }
  running_req_ = true;

  cur_sublist_.visit(td::overloaded(
      [&](const SublistGlobal &sublist) {
        send_request(td::make_tl_object<td::td_api::loadChats>(td::make_tl_object<td::td_api::chatListMain>(), 10),
                     [&](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { received_bottom_elements(std::move(R)); });
      },
      [&](const SublistArchive &sublist) {
        send_request(td::make_tl_object<td::td_api::loadChats>(td::make_tl_object<td::td_api::chatListArchive>(), 10),
                     [&](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { received_bottom_elements(std::move(R)); });
      },
      [&](const SublistSublist &sublist) {
        send_request(td::make_tl_object<td::td_api::loadChats>(
                         td::make_tl_object<td::td_api::chatListFolder>(sublist.sublist_id_), 10),
                     [&](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { received_bottom_elements(std::move(R)); });
      },
      [&](const SublistSearch &sublist) {
        send_request(td::make_tl_object<td::td_api::searchChats>(sublist.search_pattern_, 50),
                     [&](td::Result<td::tl_object_ptr<td::td_api::chats>> R) { received_bottom_chats(std::move(R)); });
      }));
}

void DialogListWindow::received_bottom_elements(td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
  //running_req_ = false;
  if (R.is_error() && R.error().code() == 404) {
    is_completed_ = true;
  }
}

void DialogListWindow::received_bottom_chats(td::Result<td::tl_object_ptr<td::td_api::chats>> R) {
  is_completed_ = true;

  if (R.is_error()) {
    return;
  }
  auto res = R.move_as_ok();
  td::int64 order = 1000000000;
  for (auto chat_id : res->chat_ids_) {
    auto chat = chat_manager().get_chat(chat_id);
    if (chat) {
      auto el = std::static_pointer_cast<Element>(std::move(chat));
      change_element(el, [&]() { el->force_update_order(--order); });
    }
  }
}

void DialogListWindow::process_update(td::td_api::updateNewChat &update) {
  auto chat = chat_manager().get_chat(update.chat_->id_);
  if (!chat) {
    auto ptr = std::make_shared<Element>(std::move(update.chat_));
    chat_manager().add_chat(ptr);
    ptr->update_order(cur_sublist_);
    if (ptr->cur_order() > 0) {
      add_element(ptr);
    }
    return;
  }

  auto el = std::static_pointer_cast<Element>(std::move(chat));
  change_element(el, [&]() {
    el->full_update(std::move(update.chat_));
    el->update_order(cur_sublist_);
  });
}

void DialogListWindow::process_update(td::td_api::updateChatLastMessage &update) {
  auto chat = chat_manager().get_chat(update.chat_id_);
  if (!chat) {
    return;
  }

  auto el = std::static_pointer_cast<Element>(std::move(chat));
  change_element(el, [&]() {
    chat_manager().process_update(update);
    el->update_order(cur_sublist_);
  });
}

void DialogListWindow::process_update(td::td_api::updateChatPosition &update) {
  auto chat = chat_manager().get_chat(update.chat_id_);
  if (!chat) {
    return;
  }

  auto el = std::static_pointer_cast<Element>(std::move(chat));
  change_element(el, [&]() {
    chat_manager().process_update(update);
    el->update_order(cur_sublist_);
  });
}

void DialogListWindow::handle_input(const windows::InputEvent &info) {
  if (info == "T-Enter") {
    auto a = get_active_element();
    if (a) {
      auto el = std::static_pointer_cast<Element>(std::move(a));
      root()->open_chat(el->chat_id());
    }
    return;
  } else if (info == "T-Escape" || info == "q" || info == "Q" || info == "C-q" || info == "C-Q") {
    root()->run_exit();
    return;
  } else if (info == "I") {
    auto a = get_active_element();
    if (a) {
      auto el = std::static_pointer_cast<Element>(std::move(a));
      create_menu_window<ChatInfoWindow>(root(), root_actor_id(), el);
    }
    return;
  } else if (info == "s") {
    spawn_chat_search_window(*this, ChatSearchWindow::Mode::Both,
                             [curses = root()](td::Result<std::shared_ptr<Chat>> R) {
                               if (R.is_ok()) {
                                 auto chat = R.move_as_ok();
                                 curses->open_chat(chat->chat_id());
                               }
                             });
    return;
  } else if (info == "/" || info == ":") {
    root()->command_line_window()->handle_input(info);
    return;
  } else if (info == "f") {
    create_menu_window<FolderSelectionWindow>(root(), root_actor_id());
    return;
  }
  return PadWindow::handle_input(info);
}

void DialogListWindow::set_search_pattern(std::string pattern) {
  if (cur_sublist_.get_offset() == cur_sublist_.offset<SublistSearch>() &&
      cur_sublist_.get<SublistSearch>().search_pattern_ == pattern) {
    return;
  }
  change_unique_id();
  if (pattern == "") {
    update_sublist(SublistGlobal{});
  } else if (pattern == "[archive]") {
    update_sublist(SublistArchive{});
  } else {
    update_sublist(SublistSearch{pattern});
  }
  running_req_ = false;
  is_completed_ = false;
  set_need_refresh();

  request_bottom_elements();

  root()->update_status_line();
}

void DialogListWindow::update_sublist(Sublist new_sublist) {
  cur_sublist_ = std::move(new_sublist);
  cur_sublist_.visit(td::overloaded([&](const SublistGlobal &sublist) { set_title("global"); },
                                    [&](const SublistArchive &sublist) { set_title("archive"); },
                                    [&](const SublistSublist &sublist) {
                                      auto &f = global_parameters().chat_folders();
                                      for (auto &s : f) {
                                        if (s->id_ == sublist.sublist_id_) {
                                          auto name = s->name_->text_->text_;
                                          set_title(PSTRING() << "sublist " << name);
                                          return;
                                        }
                                      }
                                      set_title(PSTRING() << "sublist ???");
                                    },
                                    [&](const SublistSearch &sublist) { set_title("search"); }));
  chat_manager().iterate_all_chats([&](std::shared_ptr<Chat> &chat) {
    auto el = std::static_pointer_cast<Element>(std::move(chat));
    change_element(el, [&]() {
      el->update_sublist(cur_sublist_);
      el->update_order(cur_sublist_);
    });
  });

  auto el = get_active_element();
  if (!el || !el->is_visible()) {
    scroll_first_line();
  }

  set_need_refresh();
}

void DialogListWindow::set_sublist(Sublist sublist) {
  if (cur_sublist_ == sublist) {
    return;
  }
  change_unique_id();
  update_sublist(sublist);
  running_req_ = false;
  is_completed_ = false;
  set_need_refresh();

  request_bottom_elements();

  root()->update_status_line();
}

void DialogListWindow::scroll_to_chat(td::int64 chat_id) {
  auto chat = chat_manager().get_chat(chat_id);
  if (!chat) {
    return;
  }
  auto el = std::static_pointer_cast<Element>(std::move(chat));
  scroll_to_element(el.get(), ScrollMode::Minimal);
}

}  // namespace tdcurses
