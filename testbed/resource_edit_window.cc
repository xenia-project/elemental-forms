/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * �2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions �2011-2015 Emil Seger�s: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cstdio>

#include "el/elements.h"
#include "el/io/file_manager.h"
#include "el/util/string.h"
#include "el/util/string_builder.h"
#include "testbed/resource_edit_window.h"

namespace testbed {

using namespace el;

// == ResourceItem
// ====================================================================================

ResourceItem::ResourceItem(Element* element, const std::string& str)
    : GenericStringItem(str), m_element(element) {}

// == ResourceEditWindow
// ==============================================================================

ResourceEditWindow::ResourceEditWindow()
    : m_element_list(nullptr),
      m_scroll_container(nullptr),
      m_build_container(nullptr),
      m_source_text_box(nullptr) {
  // Register as global listener to intercept events in the build container
  ElementListener::AddGlobalListener(this);

  LoadFile("resource_edit_window.tb.txt");

  m_scroll_container =
      GetElementById<ScrollContainer>(TBIDC("scroll_container"));
  m_build_container = m_scroll_container->content_root();
  m_source_text_box = GetElementById<TextBox>(TBIDC("source_edit"));

  m_element_list = GetElementById<ListBox>(TBIDC("element_list"));
  m_element_list->set_source(&m_element_list_source);

  set_rect({100, 50, 900, 600});
}

ResourceEditWindow::~ResourceEditWindow() {
  ElementListener::RemoveGlobalListener(this);

  // avoid assert
  m_element_list->set_source(nullptr);
}

void ResourceEditWindow::Load(const char* resource_file) {
  m_resource_filename = resource_file;
  set_text(resource_file);

  // Set the text of the source view.
  m_source_text_box->set_text("");

  auto file_contents = el::io::FileManager::ReadContents(m_resource_filename);
  if (file_contents) {
    m_source_text_box->set_text(
        reinterpret_cast<const char*>(file_contents->data()),
        file_contents->size());
  } else {
    // Error, show message.
    auto msg_win = new MessageForm(parent_root(), TBIDC(""));
    msg_win->Show(
        "Error loading resource",
        el::util::format_string("Could not load file %s", resource_file));
  }

  RefreshFromSource();
}

void ResourceEditWindow::RefreshFromSource() {
  // Clear old elements
  m_build_container->DeleteAllChildren();

  // Create new elements from source
  m_build_container->LoadData(m_source_text_box->text());

  // Force focus back in case the edited resource has autofocus.
  // FIX: It would be better to prevent the focus change instead!
  m_source_text_box->set_focus(FocusReason::kUnknown);
}

void ResourceEditWindow::UpdateElementList(bool immediately) {
  if (!immediately) {
    TBID id = TBIDC("update_element_list");
    if (!GetMessageById(id)) PostMessage(id, nullptr);
  } else {
    m_element_list_source.clear();
    AddElementListItemsRecursive(m_build_container, 0);

    m_element_list->InvalidateList();
  }
}

void ResourceEditWindow::AddElementListItemsRecursive(Element* element,
                                                      int depth) {
  if (depth > 0)  // Ignore the root
  {
    // Add a new ResourceItem for this element
    const char* classname = element->GetTypeName();
    if (!*classname) {
      classname = "<Unknown element type>";
    }
    auto str = el::util::format_string("% *s%s", depth - 1, "", classname);
    auto item = std::make_unique<ResourceItem>(element, str.c_str());
    m_element_list_source.push_back(std::move(item));
  }

  for (Element* child = element->first_child(); child;
       child = child->GetNext()) {
    AddElementListItemsRecursive(child, depth + 1);
  }
}

ResourceEditWindow::ITEM_INFO ResourceEditWindow::GetItemFromElement(
    Element* element) {
  ITEM_INFO item_info = {nullptr, -1};
  for (int i = 0; i < m_element_list_source.size(); i++)
    if (m_element_list_source[i]->GetElement() == element) {
      item_info.index = i;
      item_info.item = m_element_list_source[i];
      break;
    }
  return item_info;
}

void ResourceEditWindow::SetSelectedElement(Element* element) {
  m_selected_element.reset(element);
  ITEM_INFO item_info = GetItemFromElement(element);
  if (item_info.item) m_element_list->set_value(item_info.index);
}

bool ResourceEditWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kChanged &&
      ev.target->id() == TBIDC("element_list_search")) {
    m_element_list->set_filter(ev.target->text());
    return true;
  } else if (ev.type == EventType::kChanged && ev.target == m_element_list) {
    if (m_element_list->value() >= 0 &&
        m_element_list->value() < m_element_list_source.size())
      if (ResourceItem* item = m_element_list_source[m_element_list->value()])
        SetSelectedElement(item->GetElement());
  } else if (ev.type == EventType::kChanged && ev.target == m_source_text_box) {
    RefreshFromSource();
    return true;
  } else if (ev.type == EventType::kClick && ev.target->id() == TBIDC("test")) {
    // Create a window containing the current layout, resize and center it.
    auto win = new Form();
    win->set_text("Test window");
    win->content_root()->LoadData(m_source_text_box->text());
    Rect bounds(0, 0, parent()->rect().w, parent()->rect().h);
    win->set_rect(
        win->GetResizeToFitContentRect().CenterIn(bounds).MoveIn(bounds).Clip(
            bounds));
    parent()->AddChild(win);
    return true;
  } else if (ev.target->id() == TBIDC("constrained")) {
    m_scroll_container->set_adapt_content_size(ev.target->value() ? true
                                                                  : false);
    return true;
  } else if (ev.type == EventType::kFileDrop) {
    return OnDropFileEvent(ev);
  }
  return Form::OnEvent(ev);
}

void ResourceEditWindow::OnPaintChildren(const PaintProps& paint_props) {
  Form::OnPaintChildren(paint_props);

  // Paint the selection of the selected element
  if (Element* selected_element = GetSelectedElement()) {
    Rect element_rect(0, 0, selected_element->rect().w,
                      selected_element->rect().h);
    selected_element->ConvertToRoot(&element_rect.x, &element_rect.y);
    ConvertFromRoot(&element_rect.x, &element_rect.y);
    graphics::Renderer::get()->DrawRect(element_rect, Color(255, 205, 0));
  }
}

void ResourceEditWindow::OnMessageReceived(Message* msg) {
  if (msg->message_id() == TBIDC("update_element_list")) {
    UpdateElementList(true);
  }
}

bool ResourceEditWindow::OnElementInvokeEvent(Element* element,
                                              const Event& ev) {
  // Intercept all events to elements in the build container
  if (m_build_container->IsAncestorOf(ev.target)) {
    // Let events through if alt is pressed so we can test some
    // functionality right in the editor (like toggle hidden UI).
    if (any(ev.modifierkeys & ModifierKeys::kAlt)) return false;

    // Select element when clicking
    if (ev.type == EventType::kPointerDown) SetSelectedElement(ev.target);

    if (ev.type == EventType::kFileDrop) OnDropFileEvent(ev);
    return true;
  }
  return false;
}

void ResourceEditWindow::OnElementAdded(Element* parent, Element* child) {
  if (m_build_container && m_build_container->IsAncestorOf(child))
    UpdateElementList(false);
}

void ResourceEditWindow::OnElementRemove(Element* parent, Element* child) {
  if (m_build_container && m_build_container->IsAncestorOf(child))
    UpdateElementList(false);
}

bool ResourceEditWindow::OnDropFileEvent(const Event& ev) {
  auto fd_event = util::SafeCast<FileDropEvent>(&ev);
  if (fd_event->files.size() > 0) {
    Load(fd_event->files[0].c_str());
  }
  return true;
}

}  // namespace testbed
