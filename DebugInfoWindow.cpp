#include "DebugInfoWindow.hpp"
#include "Outputter.hpp"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/format.h"
#include "td/utils/overloaded.h"
#include "windows/unicode.h"
#include "TdObjectsOutput.h"
#include "td/utils/utf8.h"

namespace tdcurses {

void DebugInfoWindow::create_text(const td::td_api::message &message) {
  Outputter outputter;
  outputter << "message_id=" << message.id_ << "\n";
  outputter << "content_type=" << message.content_->get_id() << "\n";

  td::td_api::downcast_call(
      const_cast<td::td_api::MessageContent &>(*message.content_),
      td::overloaded(
          [&](const td::td_api::messageText &text) {
            auto S = td::Slice(text.text_->text_);
            size_t p = 0;

            for (td::int32 i = 0; i < 16 && p < S.size(); i++) {
              auto g = next_graphem(S, p);
              td::uint32 code;
              td::next_utf8_unsafe(g.data.ubegin(), &code);
              outputter << "GRAPHEM: width=" << g.width << " codepoints=" << g.unicode_codepoints
                        << " bytes=" << g.data.size() << " first_codepoint=" << code << " (" << g.data << ")\n";
              p += g.data.size();
            }
            for (auto &e : text.text_->entities_) {
              outputter << "ENTITY " << e->type_->get_id() << " " << e->offset_ << ":" << e->length_ << "\n";
            }

            {
              Outputter out;
              out << "AAA" << *message.content_;
              auto markup = out.markup();
              for (auto &m : markup) {
                outputter << "RESULT MARKUP: " << m->first_pos().pos << " - " << m->last_pos().pos << "\n";
              }

              auto r = out.as_str();

              auto S = td::Slice(r);
              size_t p = 0;

              for (td::int32 i = 0; i < 16 && p < S.size(); i++) {
                auto g = next_graphem(S, p);
                outputter << "GRAPHEM: width=" << g.width << " codepoints=" << g.unicode_codepoints
                          << " bytes=" << g.data.size() << " (" << g.data << ")\n";
                p += g.data.size();
              }
            }
          },
          [&](const auto &) {}));

  replace_text(outputter.as_str(), outputter.markup());
}

}  // namespace tdcurses
