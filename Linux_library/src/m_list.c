#include <common.h>
#include <m_list.h>

/**************************************************************************
name	: m_list_init
purpose	:
input	: none
output	: none
***************************************************************************/
struct m_list_s *m_list_init(void)
{
	struct m_list_s *list = NULL;

	list = (struct m_list_s *)m_malloc(sizeof(struct m_list_s));
	list->next = list;
	list->prev = list;
	list->data = NULL;

	return list;
}

/**************************************************************************
name	: __create_list_elem
purpose	: create a new list elem and assign data to this elem
input	: none
output	: none
***************************************************************************/
static struct m_list_s *__create_list_elem(void *data)
{
	struct m_list_s *elem = NULL;

	elem = (struct m_list_s *)m_malloc(sizeof(struct m_list_s));
	elem->next = NULL;
	elem->prev = NULL;
	elem->data = data;

	return elem;
}

/**************************************************************************
name	: __list_add_elem
purpose	: add new elem to list
input	: none
output	: none
***************************************************************************/
static void __list_add_elem(struct m_list_s *list,
		struct m_list_s *new)
{
	if(!list)
		return;
	(list->prev)->next = new;
	new->next = list;
	new->prev = list->prev;
	list->prev = new;
}

/**************************************************************************
name	: m_list_add
purpose	:
input	: none
output	: none
***************************************************************************/
void m_list_add(struct m_list_s *list, void *data)
{
	if(!list)
		return;
	struct m_list_s *elem = NULL;
	elem = __create_list_elem(data);
	__list_add_elem(list, elem);
}

/**************************************************************************
name	: m_list_elem_data
purpose	:
input	: none
output	: none
***************************************************************************/
void *m_list_elem_data(struct m_list_s *elem)
{
	if(!elem)
		return NULL;
	return elem->data;
}

/**************************************************************************
name	: m_list_next_elem
purpose	:
input	: none
output	: none
***************************************************************************/
struct m_list_s *m_list_next_elem(struct m_list_s *list)
{
	if(!list)
		return NULL;
	return list->next;
}

/**************************************************************************
name	: m_list_prev_elem
purpose	:
input	: none
output	: none
***************************************************************************/
struct m_list_s *m_list_prev_elem(struct m_list_s *list)
{
	if(!list)
		return NULL;
	return list->prev;
}

/*	free list elem	*/
/**************************************************************************
name	: __free_list_elem
purpose	:
input	: none
output	: none
***************************************************************************/
static void __free_list_elem(struct m_list_s *elem)
{
	if(!elem)
		return;
    m_free(elem);
    elem = NULL;
}

/**************************************************************************
name	: m_list_find_elem
purpose	:
input	: none
output	: none
***************************************************************************/
struct m_list_s *m_list_find_elem(struct m_list_s *list, void *data)
{
	m_list_t *next;

	if (data == NULL || list == NULL)
		return NULL;

	next = m_list_next_elem(list);
	while (next != list) {
		if (data == m_list_elem_data(next))
			return next;
		next = m_list_next_elem(next);
	}

	return NULL;
}

/**************************************************************************
name	: m_list_del
purpose	:
input	: none
output	: none
***************************************************************************/
void *m_list_del(struct m_list_s *del)
{
	if(!del)
		return NULL;
	void *data = NULL;

	(del->prev)->next = del->next;
	(del->next)->prev = del->prev;
	data = del->data;

	__free_list_elem(del);
	del = NULL;
	return data;
}

/**************************************************************************
name	: m_list_empty
purpose	:
input	: none
output	: none
***************************************************************************/
int m_list_empty(struct m_list_s *list)
{
	if(!list)
		return 1;
	return (list->next == list);
}

/**************************************************************************
name	: m_list_free
purpose	:
input	: none
output	: none
***************************************************************************/
void m_list_free(struct m_list_s *list)
{
	if(!list)
		return;
	void *data = NULL;
	extern void *_end;

	while(!m_list_empty(list->next)) {
		data = (void*)m_list_del(list->next);
		if(data > (void*)&_end) {
			m_free(data);
			//Free if allocated in heap
		}
		data = NULL;
	}
}

/**************************************************************************
name	: m_list_destroy
purpose	:
input	: none
output	: none
***************************************************************************/
void m_list_destroy(struct m_list_s *list)
{
	if(!list)
		return;
	m_list_free(list);
	__free_list_elem(list);
}
