#ifndef M_LIST_H_
#define M_LIST_H_

typedef struct m_list_s
{
    struct m_list_s *next, *prev;
    void *data;
} m_list_t;

/**	m_list_init. init's linked list and return a list head
 *
 * 	@return returns list (use returning parameter your list head
 */
struct m_list_s *m_list_init(void);

/**	m_list_add. add the data to the list
 *
 * 	@param list head
 * 	@param data to add list
 */
void m_list_add(struct m_list_s *list, void *data);

/**	m_list_elem_data. return list elems data
 *
 *	@param elem
 * 	@return list elems data pointer
 */
void *m_list_elem_data(struct m_list_s *elem);

/**	m_list_next_elem. get next elem of the list
 *
 *  @param list
 * 	@return next elem of list
 */
struct m_list_s *m_list_next_elem(struct m_list_s *list);

/**	m_list_prev_elem. get previus elem of the list
 *
 *  @param list
 * 	@return previus elem of list
 */
struct m_list_s *m_list_prev_elem(struct m_list_s *list);

/**	m_list_find_elem. find elem from list and returns list
 *
 * 	@param list
 *  @param data
 * 	@return find list elem data
 */
struct m_list_s *m_list_find_elem(struct m_list_s *list,
    void *data);

/**	m_list_del. delete elem from list and returns data of elem
 *
 * 	@param list elem for deletion
 * 	@return deleted list elem data
 */
void *m_list_del(struct m_list_s *del);

/**	m_list_empty. controls if list epty or not
 *
 *  @param list
 * 	@return return 1(true) if list empty
 */
int m_list_empty(struct m_list_s *list);

/**	m_list_free. free all list elems
 *
 *  @param list
 */
void m_list_free(struct m_list_s *list);

/**	m_list_destroy. destroy the list
 *
 * @param list
 */
void m_list_destroy(struct m_list_s *list);

#endif	/* _LIST_H_	*/
